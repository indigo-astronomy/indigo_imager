#include "snr_calculator.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include "indigo/indigo_bus.h"

template <typename T>
double getPixelValue(const T* data, int x, int y, int width) {
    return static_cast<double>(data[y * width + x]);
}

struct PeakInfo {
    int peak_x;
    int peak_y;
    double peak_value;
    bool valid;
};

struct CentroidInfo {
    double centroid_x;
    double centroid_y;
    bool valid;
};

struct HFDInfo {
    double hfr;
    double hfd;
    bool valid;
};

template <typename T>
PeakInfo findPeakAndDetectStar(
    const T* data,
    int width,
    int height,
    int cx,
    int cy,
    const std::vector<double>& area_pixels,
    double area_mean,
    double area_stddev,
    double click_x,
    double click_y
) {
    PeakInfo info = {cx, cy, 0, false};
    
    int star_search_radius = 8;
    double star_threshold = area_mean + 2.0 * area_stddev;
    
    bool star_detected = false;
    
    for (int dy = -star_search_radius; dy <= star_search_radius; dy++) {
        for (int dx = -star_search_radius; dx <= star_search_radius; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                double val = getPixelValue(data, px, py, width);
                if (val > info.peak_value) {
                    info.peak_value = val;
                    info.peak_x = px;
                    info.peak_y = py;
                }
                if (val > star_threshold) {
                    star_detected = true;
                }
            }
        }
    }
    
    // More robust star detection: Accept if EITHER condition is met
    double contrast_ratio = (area_mean > 0) ? (info.peak_value / area_mean) : 0;
    bool has_good_contrast = (contrast_ratio > 1.2) && (info.peak_value - area_mean > area_stddev);
    
    if (!star_detected && !has_good_contrast) {
        indigo_error("SNR: No star detected at (%.1f,%.1f) - peak=%.1f, mean=%.1f, stddev=%.1f, threshold=%.1f, contrast=%.2f",
                    click_x, click_y, info.peak_value, area_mean, area_stddev, star_threshold, contrast_ratio);
        return info;
    }
    
    indigo_error("SNR: Star detected at (%.1f,%.1f) - peak=%.1f, mean=%.1f, threshold=%.1f, contrast=%.2f, stat_detect=%d",
                click_x, click_y, info.peak_value, area_mean, star_threshold, contrast_ratio, star_detected);
    
    info.valid = true;
    return info;
}

double estimateLocalBackground(const std::vector<double>& area_pixels, double area_mean, double area_stddev) {
    double background_threshold = area_mean + area_stddev;
    std::vector<double> background_pixels;

    for (double val : area_pixels) {
        if (val < background_threshold) {
            background_pixels.push_back(val);
        }
    }

    double local_background = area_mean;
    if (!background_pixels.empty()) {
        std::sort(background_pixels.begin(), background_pixels.end());
        local_background = background_pixels[background_pixels.size() / 2];
    }
    
    return local_background;
}

template <typename T>
CentroidInfo calculateCentroid(
    const T* data,
    int width,
    int height,
    int peak_x,
    int peak_y,
    double local_background,
    double area_stddev,
    double click_x,
    double click_y
) {
    CentroidInfo info = {static_cast<double>(peak_x), static_cast<double>(peak_y), false};
    
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
        info.centroid_x = sum_x / sum_intensity;
        info.centroid_y = sum_y / sum_intensity;
    }
    
    // Multi-star detection: Check if centroid is too far from peak
    double peak_to_centroid_dist = std::sqrt(
        (info.centroid_x - peak_x) * (info.centroid_x - peak_x) +
        (info.centroid_y - peak_y) * (info.centroid_y - peak_y)
    );
    
    if (peak_to_centroid_dist > 3.0) {
        indigo_error("SNR: Centroid too far from peak (%.2f px) at (%.1f,%.1f) - likely multiple stars",
                    peak_to_centroid_dist, click_x, click_y);
        return info;
    }
    
    indigo_error("SNR: Centroid at (%.2f,%.2f), peak at (%d,%d), distance=%.2f px",
                info.centroid_x, info.centroid_y, peak_x, peak_y, peak_to_centroid_dist);
    
    info.valid = true;
    return info;
}

template <typename T>
HFDInfo calculateIterativeHFD(
    const T* data,
    int width,
    int height,
    double centroid_x,
    double centroid_y,
    double local_background
) {
    HFDInfo info = {0, 0, false};
    
    int initial_radius = 3;
    int max_iterations = 8;
    int max_radius = 50;
    
    double hfr = 0;
    double hfd = 0;
    double total_flux = 0;
    double prev_hfr = 0;
    
    for (int iteration = 0; iteration < max_iterations; iteration++) {
        std::vector<std::pair<double, double>> pixel_data;
        total_flux = 0;
        
        // Determine aperture radius for this iteration
        int aperture_radius;
        if (iteration == 0) {
            aperture_radius = initial_radius;
        } else {
            aperture_radius = static_cast<int>(hfd * 2.0 + 0.5);
            aperture_radius = std::min(aperture_radius, max_radius);
            
            if (aperture_radius < initial_radius) {
                aperture_radius = initial_radius;
            }
            
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
                    // Calculate distance from actual centroid position (sub-pixel precision)
                    double dist = std::sqrt(
                        (px - centroid_x) * (px - centroid_x) +
                        (py - centroid_y) * (py - centroid_y)
                    );
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
            return info;
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
            return info;
        }
        
        hfd = hfr * 2.0;
        
        indigo_error("SNR: Iteration %d: aperture=%d px, HFR=%.2f, HFD=%.2f, flux=%.1f",
                    iteration, aperture_radius, hfr, hfd, total_flux);

        // Stop if HFR is growing too fast (likely including neighboring stars or noise)
        if (iteration > 0 && prev_hfr > 0) {
            double hfr_ratio = hfr / prev_hfr;
            if ((hfr_ratio > 1.8 && iteration == 1) || (hfr_ratio > 1.2 && iteration > 1)) {
                indigo_error("SNR: Stopping - HFR growing too fast (%.2f -> %.2f, ratio %.2f)", 
                            prev_hfr, hfr, hfr_ratio);

                // Use previous iteration's result
                hfr = prev_hfr;
                hfd = hfr * 2.0;
                break;
            }
        }
        
        // Stop if HFR exceeds reasonable range for a star
        if (hfr > 15.0) {
            indigo_error("SNR: Stopping - HFR too large (%.2f), likely not a single star", hfr);
            // Use previous iteration's result if available
            if (iteration > 0 && prev_hfr > 0) {
                hfr = prev_hfr;
                hfd = hfr * 2.0;
            }
            break;
        }

        // Check convergence
        if (aperture_radius >= hfr * 6.0) {
            indigo_error("SNR: Converged at iteration %d (aperture >= 6*HFR)", iteration);
            break;
        }
        
        prev_hfr = hfr;
    }
    
    if (hfd <= 0 || hfr <= 0) {
        indigo_error("SNR: Invalid HFD calculated: %.2f", hfd);
        return info;
    }
    
    if (hfr < 0.5 || hfr > 20) {
        indigo_error("SNR: HFR out of range: %.2f (valid range: 0.5-20)", hfr);
        return info;
    }
    
    info.hfr = hfr;
    info.hfd = hfd;
    info.valid = true;
    return info;
}

template <typename T>
bool calculateSignalStatistics(
    const T* data,
    int width,
    int height,
    double centroid_x,
    double centroid_y,
    double star_radius,
    SNRResult& result
) {
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
        return false;
    }

    // Calculate mean of raw signal pixels
    double signal_sum = 0;
    for (double val : signal_pixels) {
        signal_sum += val;
    }
    double raw_signal_mean = signal_sum / signal_pixels.size();

    // Calculate net signal mean (background-subtracted)
    result.signal_mean = raw_signal_mean - result.background_mean;

    // For the signal error, use Poisson statistics based on total signal
    // The noise in the net signal comes from:
    // 1. Photon noise from star+background: sqrt(raw_signal_mean)
    // 2. Uncertainty in background subtraction: background_stddev
    double photon_noise = std::sqrt(std::max(0.0, raw_signal_mean));
    double bg_noise = result.background_stddev;
    result.signal_stddev = std::sqrt(photon_noise * photon_noise + bg_noise * bg_noise);
    result.star_pixels = signal_pixels.size();
    
    return true;
}

template <typename T>
bool calculateBackgroundStatistics(
    const T* data,
    int width,
    int height,
    double centroid_x,
    double centroid_y,
    double star_radius,
    int max_radius,
    SNRResult& result
) {
    double inner_radius = star_radius * 2.5;
    double outer_radius = star_radius * 4.0;
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
        return false;
    }

    std::sort(bg_annulus_pixels.begin(), bg_annulus_pixels.end());

    // Calculate median and MAD for robustness
    double bg_median = bg_annulus_pixels[bg_annulus_pixels.size() / 2];

    std::vector<double> deviations;
    for (double val : bg_annulus_pixels) {
        deviations.push_back(std::abs(val - bg_median));
    }
    std::sort(deviations.begin(), deviations.end());
    double mad = deviations[deviations.size() / 2];
    double bg_stddev_robust = 1.4826 * mad;

    // Sigma-clip: remove outliers beyond 3σ
    std::vector<double> clipped_bg;
    for (double val : bg_annulus_pixels) {
        if (std::abs(val - bg_median) < 3.0 * bg_stddev_robust) {
            clipped_bg.push_back(val);
        }
    }

    if (clipped_bg.empty()) {
        clipped_bg = bg_annulus_pixels;
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
    result.background_pixels = clipped_bg.size();
    
    return true;
}

void computeFinalSNR(SNRResult& result, double centroid_x, double centroid_y, double star_radius) {
    // Calculate total signal (sum of all pixels in aperture)
    double total_signal = result.signal_mean * result.star_pixels;
    
    // Calculate total background in star aperture
    double total_background = result.background_mean * result.star_pixels;
    
    // Net signal (background-subtracted)
    double net_signal = total_signal - total_background;
    
    // Proper aperture photometry SNR formula:
    // SNR = net_signal / sqrt(total_signal + n_pixels * background_variance)
    // 
    // This accounts for:
    // - Photon noise from the star (sqrt(total_signal))
    // - Photon noise from background (sqrt(n_pixels * background_variance))
    // - Read noise is included in background_stddev if measured from the image
    
    double background_variance = result.background_stddev * result.background_stddev;
    double noise_squared = total_signal + result.star_pixels * background_variance;
    
    if (noise_squared > 0) {
        result.snr = net_signal / std::sqrt(noise_squared);
    } else {
        result.snr = 0;
    }

    result.star_radius = star_radius;
    result.star_x = centroid_x;
    result.star_y = centroid_y;
    result.valid = (result.snr > 0 && result.snr < 10000);
    
    indigo_error("SNR: Final - total_signal=%.1f, net_signal=%.1f, noise=%.1f, SNR=%.2f",
                total_signal, net_signal, std::sqrt(noise_squared), result.snr);
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

    // Step 2: Find peak and detect star
    PeakInfo peak = findPeakAndDetectStar(data, width, height, cx, cy, area_pixels, 
                                          area_mean, area_stddev, click_x, click_y);
    if (!peak.valid) {
        return result;
    }

    // Step 3: Estimate local background
    double local_background = estimateLocalBackground(area_pixels, area_mean, area_stddev);

    // Step 4: Calculate centroid
    CentroidInfo centroid = calculateCentroid(data, width, height, peak.peak_x, peak.peak_y,
                                              local_background, area_stddev, click_x, click_y);
    if (!centroid.valid) {
        return result;
    }

    // Step 5: Iterative HFD calculation
    HFDInfo hfd_info = calculateIterativeHFD(data, width, height, centroid.centroid_x, 
                                             centroid.centroid_y, local_background);
    if (!hfd_info.valid) {
        return result;
    }

    // Star aperture radius = 3.0 * HFR (captures ~94% of flux)
    double star_radius = hfd_info.hfr * 3.0;
    star_radius = std::min(star_radius, 25.0);

    // Step 6: Calculate signal statistics
    if (!calculateSignalStatistics(data, width, height, centroid.centroid_x, centroid.centroid_y,
                                   star_radius, result)) {
        return result;
    }

    // Step 7: Calculate background statistics
    int max_radius = 50;
    if (!calculateBackgroundStatistics(data, width, height, centroid.centroid_x, centroid.centroid_y,
                                       star_radius, max_radius, result)) {
        return result;
    }

    // Step 8: Compute final SNR
    computeFinalSNR(result, centroid.centroid_x, centroid.centroid_y, star_radius);
    
    // Store HFD value
    result.hfd = hfd_info.hfd;

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