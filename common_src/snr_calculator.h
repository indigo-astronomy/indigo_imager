#ifndef SNR_CALCULATOR_H
#define SNR_CALCULATOR_H

#include <cstdint>
#include <pixelformat.h>

struct SNRResult {
    double snr;
    double signal_mean;
    double signal_stddev;
    double background_mean;
    double background_stddev;
    int star_pixels;
    int background_pixels;
    double star_radius;
    double star_x;
    double star_y;
    double hfd;  // Half Flux Diameter
    bool valid;

    SNRResult() : snr(0), signal_mean(0), signal_stddev(0),
                  background_mean(0), background_stddev(0),
                  star_pixels(0), background_pixels(0),
                  star_radius(0), star_x(0), star_y(0), hfd(0), valid(false) {}
};

// Calculate SNR for a star at given coordinates
// Automatically detects star radius and calculates SNR
SNRResult calculateSNR(
    const uint8_t *image_data,
    int width,
    int height,
    int pix_fmt,
    double click_x,
    double click_y,
    int search_radius = 20  // Search area for star centroid
);

#endif // SNR_CALCULATOR_H