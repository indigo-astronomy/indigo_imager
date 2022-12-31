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
	sum = 0;
	for (int i = 0; i < count; i++) {
		d = buffer[i] - mean;
		sum += d * d;
	}
	double stddev = sqrt(sum / count);

	stats.channels = 1;
	stats.grey_red.min = min;
	stats.grey_red.max = max;
	stats.grey_red.mean = mean;
	stats.grey_red.stddev = stddev;
	if (stats.grey_red.stddev != 0) {
		stats.grey_red.SNR = mean / stats.grey_red.stddev;
	} else {
		stats.grey_red.SNR = INFINITY;
	}
	return stats;
}

template <typename T>
ImageStats imageStatsThreeChannels(T const *buffer, int width, int height) {
	/*
	double d, m, sum = 0;
	int real_count = 0;
	int i = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			i = (y * width + x) * 3;
			sum += set[i];
			sum += set[i + 1];
			sum += set[i + 2];
			real_count++;
		}
	}
	m = sum / (real_count * 3);

	const double threshold = (SATURATION_8 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			i = (y * width + x) * 3;
			if (
				(
					set[i] > SATURATION_8 ||
					set[i + 1] > SATURATION_8 ||
					set[i + 2] > SATURATION_8
				) && (
					median(set[i - 3], set[i], set[i + 3]) > threshold ||
					median(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
					median(set[i - 1], set[i + 2], set[i + 5]) > threshold
				)
			) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, mean = %.2f", threshold, m));
					*saturated = true;
				}
			}
			d = set[i] - m;
			sum += d * d;
			d = set[i + 1] - m;
			sum += d * d;
			d = set[i + 2] - m;
			sum += d * d;

			real_count++;
		}
	}

	return sqrt(sum / real_count);
	*/
	ImageStats stats;
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
			return imageStatsThreeChannels(reinterpret_cast<uint8_t const*>(input), width, height);
		case PIX_FMT_RGB48:
			return imageStatsThreeChannels(reinterpret_cast<uint16_t const*>(input), width, height);
		case PIX_FMT_RGB96:
			return imageStatsThreeChannels(reinterpret_cast<uint32_t const*>(input), width, height);
		case PIX_FMT_RGBF:
			return imageStatsThreeChannels(reinterpret_cast<float const*>(input), width, height);
		default:
			return ImageStats();
	}
}
