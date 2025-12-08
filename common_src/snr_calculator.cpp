#include "snr_calculator.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include "indigo/indigo_bus.h"

template <typename T>
double getPixelValue(const T* data, int x, int y, int width) {
    return static_cast<double>(data[y * width + x]);
}

template <typename T>
SNRResult calculateSNRTemplate(
    const T *data,
    int width,
    int height,
    double click_x,
    double click_y,
    int search_radius
) {
    SNRResult result;

    int cx = static_cast<int>(click_x);
    int cy = static_cast<int>(click_y);

    // Fixed 8px search radius around click point to find star
    int star_search_radius = 8;

    // Step 1: Calculate statistics of the 8px search area
    std::vector<double> area_pixels;

    for (int dy = -star_search_radius; dy <= star_search_radius; dy++) {
        for (int dx = -star_search_radius; dx <= star_search_radius; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                area_pixels.push_back(getPixelValue(data, px, py, width));
            }
        }
    }

    if (area_pixels.empty()) {
        return result;
    }

    // Calculate mean and standard deviation of search area
    double area_sum = 0;
    for (double val : area_pixels) {
        area_sum += val;
    }
    double area_mean = area_sum / area_pixels.size();

    double area_variance = 0;
    for (double val : area_pixels) {
        double diff = val - area_mean;
        area_variance += diff * diff;
    }
    double area_stddev = std::sqrt(area_variance / area_pixels.size());

    // Star detection threshold: mean + 2σ (was 3σ - too strict for faint stars)
    // This is more forgiving while still rejecting pure noise
    double star_threshold = area_mean + 2.0 * area_stddev;

    // Step 2: Find peak pixel within 8px radius that exceeds 2σ threshold
    double peak_value = 0;
    int peak_x = cx;
    int peak_y = cy;
    bool star_detected = false;

    for (int dy = -star_search_radius; dy <= star_search_radius; dy++) {
        for (int dx = -star_search_radius; dx <= star_search_radius; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                double val = getPixelValue(data, px, py, width);
                if (val > peak_value) {
                    peak_value = val;
                    peak_x = px;
                    peak_y = py;
                }
                if (val > star_threshold) {
                    star_detected = true;
                }
            }
        }
    }

    // More robust star detection: Accept if EITHER condition is met:
    // 1. Statistical threshold (mean + 2σ) is exceeded, OR
    // 2. Peak has good contrast: at least 20% brighter than area mean and mean > 0
    double contrast_ratio = (area_mean > 0) ? (peak_value / area_mean) : 0;
    bool has_good_contrast = (contrast_ratio > 1.2) && (peak_value - area_mean > area_stddev);
    
    if (!star_detected && !has_good_contrast) {
        indigo_error("SNR: No star detected at (%.1f,%.1f) - peak=%.1f, mean=%.1f, stddev=%.1f, threshold=%.1f, contrast=%.2f",
                    click_x, click_y, peak_value, area_mean, area_stddev, star_threshold, contrast_ratio);
        return result; // No significant star detected in 8px radius
    }
    
    indigo_error("SNR: Star detected at (%.1f,%.1f) - peak=%.1f, mean=%.1f, threshold=%.1f, contrast=%.2f, stat_detect=%d",
                click_x, click_y, peak_value, area_mean, star_threshold, contrast_ratio, star_detected);

    // Step 3: Estimate local background using robust sigma-clipped mean
    // Use pixels below mean + 1σ (likely background pixels)
    double background_threshold = area_mean + area_stddev;
    std::vector<double> background_pixels;

    for (double val : area_pixels) {
        if (val < background_threshold) {
            background_pixels.push_back(val);
        }
    }

    double local_background = area_mean;
    if (!background_pixels.empty()) {
        // Use median of background pixels for robustness
        std::sort(background_pixels.begin(), background_pixels.end());
        local_background = background_pixels[background_pixels.size() / 2];
    }

    // Step 4: Calculate centroid around peak (small radius to avoid neighboring stars)
    double centroid_x = peak_x;
    double centroid_y = peak_y;
    
    int centroid_radius = 8;
    double centroid_threshold = local_background + area_stddev;
    
    double sum_intensity = 0;
    double sum_x = 0;
    double sum_y = 0;
    
    for (int dy = -centroid_radius; dy <= centroid_radius; dy++) {
        for (int dx = -centroid_radius; dx <= centroid_radius; dx++) {
            int px = peak_x + dx;
            int py = peak_y + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= centroid_radius) {
                    double val = getPixelValue(data, px, py, width);
                    if (val > centroid_threshold) {
                        double weight = val - local_background;
                        if (weight > 0) {
                            sum_intensity += weight;
                            sum_x += px * weight;
                            sum_y += py * weight;
                        }
                    }
                }
            }
        }
    }
    
    if (sum_intensity > 0) {
        centroid_x = sum_x / sum_intensity;
        centroid_y = sum_y / sum_intensity;
    }
    
    // Multi-star detection: Check if centroid is too far from peak
    double peak_to_centroid_dist = std::sqrt(
        (centroid_x - peak_x) * (centroid_x - peak_x) +
        (centroid_y - peak_y) * (centroid_y - peak_y)
    );
    
    // If centroid is more than 3 pixels away from peak, likely multiple stars pulling it
    if (peak_to_centroid_dist > 3.0) {
        indigo_error("SNR: Centroid too far from peak (%.2f px) at (%.1f,%.1f) - likely multiple stars",
                    peak_to_centroid_dist, click_x, click_y);
        return result;
    }
    
    indigo_error("SNR: Centroid at (%.2f,%.2f), peak at (%d,%d), distance=%.2f px",
                centroid_x, centroid_y, peak_x, peak_y, peak_to_centroid_dist);

    // Step 5: Iterative HFD calculation with growing aperture
    // Start with small aperture and grow up to 2-3x the measured HFD
    double hfr = 0;
    double hfd = 0;
    double total_flux = 0;
    
    // Start with smaller initial aperture to avoid including neighboring stars
    int initial_radius = 3;  // Changed from 15 to 3
    int max_iterations = 8;   // Increased to allow more growth steps
    int max_radius = 50;
    
    for (int iteration = 0; iteration < max_iterations; iteration++) {
        std::vector<std::pair<double, double>> pixel_data;
        total_flux = 0;
        
        // Determine aperture radius for this iteration
        int aperture_radius;
        if (iteration == 0) {
            aperture_radius = initial_radius;
        } else {
            // Grow aperture more conservatively: 2.0x the current HFD (was 2.5x)
            aperture_radius = static_cast<int>(hfd * 2.0 + 0.5);  // Round up
            aperture_radius = std::min(aperture_radius, max_radius); // Cap at 50px
            
            // Don't shrink the aperture
            if (aperture_radius < initial_radius) {
                aperture_radius = initial_radius;
            }
            
            // If aperture didn't grow from last iteration, we're done
            static int last_aperture = 0;
            if (iteration > 1 && aperture_radius == last_aperture) {
                indigo_error("SNR: Converged at iteration %d (aperture stable)", iteration);
                break;
            }
            last_aperture = aperture_radius;
        }
        
        // Collect pixels in circular aperture around centroid
        for (int dy = -aperture_radius; dy <= aperture_radius; dy++) {
            for (int dx = -aperture_radius; dx <= aperture_radius; dx++) {
                int px = static_cast<int>(centroid_x) + dx;
                int py = static_cast<int>(centroid_y) + dy;
                
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    double dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= aperture_radius) {
                        double val = getPixelValue(data, px, py, width);
                        double above_bg = val - local_background;
                        
                        if (above_bg > 0) {
                            pixel_data.push_back(std::make_pair(dist, above_bg));
                            total_flux += above_bg;
                        }
                    }
                }
            }
        }
        
        if (total_flux <= 0 || pixel_data.empty()) {
            indigo_error("SNR: No flux found in iteration %d", iteration);
            return result;
        }
        
        // Sort by distance for HFR calculation
        std::sort(pixel_data.begin(), pixel_data.end());
        
        // Calculate HFR (Half Flux Radius)
        double half_flux = total_flux / 2.0;
        double accumulated_flux = 0;
        hfr = 0;
        
        for (const auto& pd : pixel_data) {
            accumulated_flux += pd.second;
            if (accumulated_flux >= half_flux) {
                hfr = pd.first;
                break;
            }
        }
        
        // Validate: HFR should not exceed aperture radius
        if (hfr > aperture_radius) {
            indigo_error("SNR: HFR (%.2f) exceeds aperture (%d) - likely bad data or multiple stars", 
                        hfr, aperture_radius);
            return result;
        }
        
        hfd = hfr * 2.0;
        
        indigo_error("SNR: Iteration %d: aperture=%d px, HFR=%.2f, HFD=%.2f, flux=%.1f",
                    iteration, aperture_radius, hfr, hfd, total_flux);

        // Check convergence: if aperture is already 4x HFD or larger, we're done
        if (aperture_radius >= hfd * 4.0) {
            indigo_error("SNR: Converged at iteration %d (aperture >= 4*HFD)", iteration);
            break;
        }
    }
    
    if (hfd <= 0 || hfr <= 0) {
        indigo_error("SNR: Invalid HFD calculated: %.2f", hfd);
        return result;
    }
    
    // Sanity check - reject if HFR is unreasonable
    if (hfr < 0.5 || hfr > 20) {
        indigo_error("SNR: HFR out of range: %.2f (valid range: 0.5-20)", hfr);
        return result;
    }

    // Star aperture radius = 3.0 * HFR (captures ~94% of flux)
    double star_radius = hfr * 3.0;
    
    // Additional safety limit: cap star radius at 25 pixels
    star_radius = std::min(star_radius, 25.0);

    // Step 6: Calculate signal statistics in star aperture
    std::vector<double> signal_pixels;
    int aperture_left = std::max(0, static_cast<int>(centroid_x - star_radius - 1));
    int aperture_right = std::min(width - 1, static_cast<int>(centroid_x + star_radius + 1));
    int aperture_top = std::max(0, static_cast<int>(centroid_y - star_radius - 1));
    int aperture_bottom = std::min(height - 1, static_cast<int>(centroid_y + star_radius + 1));

    for (int y = aperture_top; y <= aperture_bottom; y++) {
        for (int x = aperture_left; x <= aperture_right; x++) {
            double dist = std::sqrt(
                (x - centroid_x) * (x - centroid_x) +
                (y - centroid_y) * (y - centroid_y)
            );
            if (dist <= star_radius) {
                signal_pixels.push_back(getPixelValue(data, x, y, width));
            }
        }
    }

    if (signal_pixels.empty()) {
        return result;
    }

    double signal_sum = 0;
    for (double val : signal_pixels) {
        signal_sum += val;
    }
    result.signal_mean = signal_sum / signal_pixels.size();

    double signal_var = 0;
    for (double val : signal_pixels) {
        double diff = val - result.signal_mean;
        signal_var += diff * diff;
    }
    result.signal_stddev = std::sqrt(signal_var / signal_pixels.size());

    // Step 7: Calculate background in annulus
    double inner_radius = star_radius * 1.5;
    double outer_radius = star_radius * 3.0;
    outer_radius = std::min(outer_radius, static_cast<double>(max_radius));

    std::vector<double> bg_annulus_pixels;
    int bg_left = std::max(0, static_cast<int>(centroid_x - outer_radius - 1));
    int bg_right = std::min(width - 1, static_cast<int>(centroid_x + outer_radius + 1));
    int bg_top = std::max(0, static_cast<int>(centroid_y - outer_radius - 1));
    int bg_bottom = std::min(height - 1, static_cast<int>(centroid_y + outer_radius + 1));

    for (int y = bg_top; y <= bg_bottom; y++) {
        for (int x = bg_left; x <= bg_right; x++) {
            double dist = std::sqrt(
                (x - centroid_x) * (x - centroid_x) +
                (y - centroid_y) * (y - centroid_y)
            );
            if (dist >= inner_radius && dist <= outer_radius) {
                bg_annulus_pixels.push_back(getPixelValue(data, x, y, width));
            }
        }
    }

    if (bg_annulus_pixels.empty()) {
        return result;
    }

    // Use sigma-clipped mean for robust background estimation
    std::sort(bg_annulus_pixels.begin(), bg_annulus_pixels.end());

    // Calculate median and MAD (Median Absolute Deviation) for robustness
    double bg_median = bg_annulus_pixels[bg_annulus_pixels.size() / 2];

    std::vector<double> deviations;
    for (double val : bg_annulus_pixels) {
        deviations.push_back(std::abs(val - bg_median));
    }
    std::sort(deviations.begin(), deviations.end());
    double mad = deviations[deviations.size() / 2];
    double bg_stddev_robust = 1.4826 * mad; // Convert MAD to stddev equivalent

    // Sigma-clip: remove outliers beyond 3σ
    std::vector<double> clipped_bg;
    for (double val : bg_annulus_pixels) {
        if (std::abs(val - bg_median) < 3.0 * bg_stddev_robust) {
            clipped_bg.push_back(val);
        }
    }

    // Calculate final background statistics from clipped data
    if (clipped_bg.empty()) {
        clipped_bg = bg_annulus_pixels; // Fallback if all clipped
    }

    double bg_sum = 0;
    for (double val : clipped_bg) {
        bg_sum += val;
    }
    result.background_mean = bg_sum / clipped_bg.size();

    double bg_var = 0;
    for (double val : clipped_bg) {
        double diff = val - result.background_mean;
        bg_var += diff * diff;
    }
    result.background_stddev = std::sqrt(bg_var / clipped_bg.size());

    // Step 8: Calculate SNR
    double net_signal = result.signal_mean - result.background_mean;
    double noise = result.background_stddev;

    if (noise > 0) {
        result.snr = net_signal / noise;
    }

    result.star_pixels = signal_pixels.size();
    result.background_pixels = clipped_bg.size();
    result.star_radius = star_radius;
    result.star_x = centroid_x;
    result.star_y = centroid_y;
    result.valid = (result.snr > 0 && result.snr < 1000);

    return result;
}

SNRResult calculateSNR(
    const uint8_t *image_data,
    int width,
    int height,
    int pix_fmt,
    double click_x,
    double click_y,
    int search_radius
) {
    switch (pix_fmt) {
        case PIX_FMT_Y8:
            return calculateSNRTemplate(
                reinterpret_cast<const uint8_t*>(image_data),
                width, height, click_x, click_y, search_radius
            );
        case PIX_FMT_Y16:
            return calculateSNRTemplate(
                reinterpret_cast<const uint16_t*>(image_data),
                width, height, click_x, click_y, search_radius
            );
        case PIX_FMT_Y32:
            return calculateSNRTemplate(
                reinterpret_cast<const uint32_t*>(image_data),
                width, height, click_x, click_y, search_radius
            );
        case PIX_FMT_F32:
            return calculateSNRTemplate(
                reinterpret_cast<const float*>(image_data),
                width, height, click_x, click_y, search_radius
            );
        default:
            return SNRResult();
    }
}