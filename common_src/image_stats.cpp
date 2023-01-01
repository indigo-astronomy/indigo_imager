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

#include "image_stats.h"

#include <math.h>
#include <QCoreApplication>
#include <QtConcurrent>

#define MAX_THREADS 4

template <typename T>
ImageStats imageStatsOneChannel(T const *buffer, int count) {
	ImageStats stats;
	if (count < 1) return stats;

	double sum = 0, min = INFINITY, max = 0;
	for (int i = 0; i < count; i++) {
		sum += buffer[i];
		if (buffer[i] > max) max = buffer[i];
		if (buffer[i] < min) min = buffer[i];
	}
	double mean = sum / count;

	double d;
	double stddev_sum = 0, mad_sum = 0;
	for (int i = 0; i < count; i++) {
		d = buffer[i] - mean;
		stddev_sum += d * d;
		mad_sum += fabs(d);
	}
	double stddev = sqrt(stddev_sum / count);
	double mad = mad_sum / count;

	stats.channels = 1;
	stats.grey_red.min = min;
	stats.grey_red.max = max;
	stats.grey_red.mean = mean;
	stats.grey_red.stddev = stddev;
	stats.grey_red.mad = mad;
	return stats;
}

template <typename T>
ImageStats imageStatsThreeChannels(T const *buffer, int count) {
	ImageStats stats;
	if (count < 3) return stats;

	double sum_r = 0, sum_g = 0, sum_b = 0;
	double min_r = INFINITY, min_g = INFINITY, min_b = INFINITY;
	double max_r = 0, max_g = 0, max_b = 0;

	for (int i = 0; i < count * 3; i += 3) {
		sum_r += buffer[i];
		if (buffer[i] > max_r) max_r = buffer[i];
		if (buffer[i] < min_r) min_r = buffer[i];

		sum_g += buffer[i+1];
		if (buffer[i + 1] > max_g) max_g = buffer[i + 1];
		if (buffer[i + 1] < min_g) min_g = buffer[i + 1];

		sum_b += buffer[i + 2];
		if (buffer[i + 2] > max_b) max_b = buffer[i + 2];
		if (buffer[i + 2] < min_b) min_b = buffer[i + 2];
	}
	double mean_r = sum_r / count;
	double mean_g = sum_g / count;
	double mean_b = sum_b / count;

	double d;
	double stddev_sum_r = 0, stddev_sum_g = 0, stddev_sum_b = 0;
	double mad_sum_r = 0, mad_sum_g = 0, mad_sum_b = 0;
	for (int i = 0; i < count * 3; i += 3) {
		d = buffer[i] - mean_r;
		stddev_sum_r += d * d;
		mad_sum_r += fabs(d);

		d = buffer[i + 1] - mean_g;
		stddev_sum_g += d * d;
		mad_sum_g += fabs(d);

		d = buffer[i + 2] - mean_b;
		stddev_sum_b += d * d;
		mad_sum_b += fabs(d);
	}
	double stddev_r = sqrt(stddev_sum_r / count);
	double stddev_g = sqrt(stddev_sum_g / count);
	double stddev_b = sqrt(stddev_sum_b / count);
	double mad_r = mad_sum_r / count;
	double mad_g = mad_sum_g / count;
	double mad_b = mad_sum_b / count;

	stats.channels = 3;

	stats.grey_red.min = min_r;
	stats.grey_red.max = max_r;
	stats.grey_red.mean = mean_r;
	stats.grey_red.stddev = stddev_r;
	stats.grey_red.mad = mad_r;

	stats.green.min = min_g;
	stats.green.max = max_g;
	stats.green.mean = mean_g;
	stats.green.stddev = stddev_g;
	stats.green.mad = mad_g;

	stats.blue.min = min_b;
	stats.blue.max = max_b;
	stats.blue.mean = mean_b;
	stats.blue.stddev = stddev_b;
	stats.blue.mad = mad_b;

	return stats;
}

ImageStats imageStats(uint8_t const *input, int width, int height, int pix_fmt) {
	switch (pix_fmt) {
		case PIX_FMT_Y8:
			return imageStatsOneChannel(reinterpret_cast<uint8_t const*>(input), width * height);
		case PIX_FMT_Y16:
			return imageStatsOneChannel(reinterpret_cast<uint16_t const*>(input), height * width);
		case PIX_FMT_Y32:
			return imageStatsOneChannel(reinterpret_cast<uint32_t const*>(input), height * width);
		case PIX_FMT_F32:
			return imageStatsOneChannel(reinterpret_cast<float const*>(input), height * width);
		case PIX_FMT_RGB24:
			return imageStatsThreeChannels(reinterpret_cast<uint8_t const*>(input), width * height);
		case PIX_FMT_RGB48:
			return imageStatsThreeChannels(reinterpret_cast<uint16_t const*>(input), width * height);
		case PIX_FMT_RGB96:
			return imageStatsThreeChannels(reinterpret_cast<uint32_t const*>(input), width * height);
		case PIX_FMT_RGBF:
			return imageStatsThreeChannels(reinterpret_cast<float const*>(input), width * height);
		default:
			return ImageStats();
	}
}
