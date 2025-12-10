#ifndef SNR_CALCULATOR_H
#define SNR_CALCULATOR_H

#include <cstdint>
#include <string>
#include <pixelformat.h>

struct SNRResult {
	double snr;
	double hfd;
	double signal_mean;
	double signal_stddev;
	double background_mean;
	double background_stddev;
	int star_pixels;
	int background_pixels;
	double star_radius;
	double star_x;
	double star_y;
	double background_inner_radius;
	double background_outer_radius;
	double eccentricity;  // Star roundness: 0=perfect circle, 1=linear
	double peak_value;    // Maximum pixel value in star
	double total_flux;    // Total background-subtracted flux
	bool valid;
	bool is_saturated;
	std::string error_message;  // Error description if valid == false

	SNRResult() :
		snr(0), hfd(0), signal_mean(0), signal_stddev(0),
		background_mean(0), background_stddev(0),
		star_pixels(0), background_pixels(0),
		star_radius(0), star_x(0), star_y(0),
		background_inner_radius(0), background_outer_radius(0),
		eccentricity(0), peak_value(0), total_flux(0),
		valid(false), is_saturated(false), error_message("")
	{}

};

// Calculate SNR for a star at given coordinates
// Automatically detects star radius and calculates SNR
SNRResult calculateSNR(
	const uint8_t *image_data,
	int width,
	int height,
	int pix_fmt,
	double click_x,
	double click_y
);

#endif // SNR_CALCULATOR_H