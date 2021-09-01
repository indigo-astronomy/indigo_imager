// based on https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html

#pragma once

#include <memory>
#include <QImage>

#define TBYTE        11 /* codes for FITS table data types */
#define TSBYTE       12
#define TUSHORT      20
#define TSHORT       21

struct StretchParams1Channel {
 	// Stretch algorithm parameters
 	float shadows;;
 	float highlights;
 	float midtones;
 	// The extension parameters are not used.
 	float shadows_expansion;
 	float highlights_expansion;

 	// The default parameters result in no stretch at all.
 	StretchParams1Channel() {
		shadows = 0.0;
		highlights = 1.0;
		midtones = 0.5;
		shadows_expansion = 0.0;
		highlights_expansion = 1.0;
 	}
};

struct StretchParams {
 	StretchParams1Channel grey_red;
 	StretchParams1Channel green;
 	StretchParams1Channel blue;
};

class Stretcher {
public:
	explicit Stretcher(int width, int height, int channels, int data_type);
	~Stretcher() {}

	void setParams(StretchParams input_params) { params = input_params; }
	StretchParams getParams() { return params; }
	StretchParams computeParams(const uint8_t *input);
	void stretch(uint8_t const *input, QImage *output_image, int sampling=1);

 private:
	 int image_width;
	 int image_height;
	 int image_channels;
	 int input_range;
	 int dataType;

	 StretchParams params;
};
