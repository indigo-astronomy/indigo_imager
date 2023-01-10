// Copyright (c) 2023 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <memory>
#include <pixelformat.h>
#include <QImage>

const int hist_height = 128;
const int hist_width = 256;

struct ImageStats1Channel {
	double min;
	double max;
	double mean;
	double stddev;
	double mad;
	uint32_t histogram[hist_width];

	ImageStats1Channel() {
		min =
		max =
		mean =
		stddev =
		mad = 0;
		for (int i = 0; i < hist_width; i++) histogram[i] = 0;
	}
};

struct ImageStats {
	int channels;
	int pix_fmt;
	int8_t bitdepth;
	ImageStats1Channel grey_red;
	ImageStats1Channel green;
	ImageStats1Channel blue;

	// 0 - uninitialized, 1 - monochrome, 3 - RGB image
	ImageStats() {
		channels =
		pix_fmt =
		bitdepth = 0;
	}
};

//ImageStats imageStats(uint8_t const *input, int width, int height, int pix_fmt);
ImageStats imageStats(uint8_t const *input, int width, int height, int pix_fmt, int8_t bitdepth_hint = 0);
QImage makeHistogram(ImageStats stats);