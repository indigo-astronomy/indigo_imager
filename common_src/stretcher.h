// based on https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html

#pragma once

#include <memory>
#include <QImage>
#include <pixelformat.h>

#define DEFAULT_B (0.25)
#define DEFAULT_C (-2.8)

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
	StretchParams1Channel *refChannel;
	StretchParams1Channel grey_red;
	StretchParams1Channel green;
	StretchParams1Channel blue;

	StretchParams() {
		refChannel = nullptr;
	}
};

class Stretcher {
public:
	explicit Stretcher(int width, int height, int pix_fmt);
	~Stretcher() {}

	void setParams(StretchParams input_params) { m_params = input_params; }
	StretchParams getParams() { return m_params; }
	StretchParams computeParams(const uint8_t *input, const float B = DEFAULT_B, const float C = DEFAULT_C);
	void stretch(uint8_t const *input, QImage *output_image, int sampling=1);

private:
	int m_image_width;
	int m_image_height;
	int m_input_range;
	uint32_t m_pix_fmt;

	StretchParams m_params;
};
