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

    // Star detection threshold: mean + 3σ
    double star_threshold = area_mean + 3.0 * area_stddev;

    // Step 2: Find peak pixel within 8px radius that exceeds 3σ threshold
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

    // Check if peak exceeds 3σ threshold
    if (!star_detected || peak_value < star_threshold) {
        return result; // No significant star detected in 8px radius
    }

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

    // Step 4: Calculate intensity-weighted centroid (iterative refinement)
    double centroid_x = peak_x;
    double centroid_y = peak_y;

    // Iterate to refine centroid within 8px radius
    for (int iter = 0; iter < 5; iter++) {
        double sum_intensity = 0;
        double sum_x = 0;
        double sum_y = 0;
        int count = 0;

        // Fixed radius of 8 pixels for centroid calculation
        int refine_radius = 8;

        // Use statistical threshold: mean + 2σ for centroid calculation
        double centroid_threshold = area_mean + 2.0 * area_stddev;

        for (int dy = -refine_radius; dy <= refine_radius; dy++) {
            for (int dx = -refine_radius; dx <= refine_radius; dx++) {
                int px = static_cast<int>(centroid_x) + dx;
                int py = static_cast<int>(centroid_y) + dy;
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    double val = getPixelValue(data, px, py, width);
                    // Only use pixels above threshold
                    if (val > centroid_threshold) {
                        double weight = val - local_background;
                        if (weight > 0) {
                            sum_intensity += weight;
                            sum_x += px * weight;
                            sum_y += py * weight;
                            count++;
                        }
                    }
                }
            }
        }

        if (sum_intensity == 0 || count < 3) {
            return result;
        }

        double new_centroid_x = sum_x / sum_intensity;
        double new_centroid_y = sum_y / sum_intensity;

        // Check convergence
        double shift = std::sqrt(
            (new_centroid_x - centroid_x) * (new_centroid_x - centroid_x) +
            (new_centroid_y - centroid_y) * (new_centroid_y - centroid_y)
        );

        centroid_x = new_centroid_x;
        centroid_y = new_centroid_y;

        if (shift < 0.01) break; // Converged to 0.01 pixel accuracy
    }

    // Step 5: Calculate HFD using background-subtracted flux
    // Use larger radius for flux calculation (up to 50px)
    int max_radius = 50;
    std::vector<std::pair<double, double>> pixel_data;
    double total_flux = 0;

    // Detection threshold for flux calculation: mean + 1σ
    double flux_threshold = area_mean + area_stddev;

    // Collect pixels in circular aperture around refined centroid
    int collect_left = std::max(0, static_cast<int>(centroid_x - max_radius));
    int collect_right = std::min(width - 1, static_cast<int>(centroid_x + max_radius));
    int collect_top = std::max(0, static_cast<int>(centroid_y - max_radius));
    int collect_bottom = std::min(height - 1, static_cast<int>(centroid_y + max_radius));

    for (int y = collect_top; y <= collect_bottom; y++) {
        for (int x = collect_left; x <= collect_right; x++) {
            double dist = std::sqrt(
                (x - centroid_x) * (x - centroid_x) +
                (y - centroid_y) * (y - centroid_y)
            );
            if (dist <= max_radius) {
                double val = getPixelValue(data, x, y, width);
                // Only include pixels above noise threshold
                if (val > flux_threshold) {
                    double above_bg = val - local_background;

                    if (above_bg > 0) {
                        pixel_data.push_back(std::make_pair(dist, above_bg));
                        total_flux += above_bg;
                    }
                }
            }
        }
    }

    if (pixel_data.empty() || total_flux <= 0) {
        return result;
    }

    // Sort by distance
    std::sort(pixel_data.begin(), pixel_data.end(),
        [](const std::pair<double, double>& a, const std::pair<double, double>& b) {
            return a.first < b.first;
        });

    // Find HFR (Half Flux Radius)
    double cumulative_flux = 0;
    double half_flux = total_flux * 0.5;
    double hfr = 0;

    for (const auto& pixel : pixel_data) {
        cumulative_flux += pixel.second;
        if (cumulative_flux >= half_flux) {
            hfr = pixel.first;
            break;
        }
    }

    if (hfr <= 0 || hfr > max_radius) {
        return result;
    }

    // Sanity check
    if (hfr < 0.5 || hfr > 50) {
        return result;
    }

    // Star aperture radius = 2.0 * HFR (captures ~94% of flux)
    double star_radius = hfr * 2.0;
    star_radius = std::min(star_radius, static_cast<double>(max_radius));

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