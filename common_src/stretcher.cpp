// based on https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html

#include "stretcher.h"
//#include <indigo/indigo_bus.h>
//#include <indigo/indigo_raw_utils.h>

#include <math.h>
#include <QCoreApplication>
#include <QtConcurrent>
#include <utils.h>

#define DEFAULT_THREADS 4

// Returns the median value of the vector.
// The values is modified
template <typename T>
T median(std::vector<T> &values) {
	const int middle = values.size() / 2;
	std::nth_element(values.begin(), values.begin() + middle, values.end());
	return values[middle];
}

// Returns the median of the sample values.
// The values not modified
template <typename T>
T median(T const *values, int size, int sampleBy) {
	const int downsampled_size = size / sampleBy;
	std::vector<T> samples(downsampled_size);
	for (int index = 0, i = 0; i < downsampled_size; ++i, index += sampleBy) {
		samples[i] = values[index];
	}
	return median(samples);
}

template <typename T>
void stretchOneChannel(
	T *input_buffer,
	QImage *output_image,
	const StretchParams &stretch_params,
	double input_range,
	int image_height,
	int image_width,
	int sampling
) {
	constexpr int maxOutput = 255;

	const double maxInput = input_range > 1 ? input_range - 1 : input_range;

	const float midtones   = stretch_params.grey_red.midtones;
	const float highlights = stretch_params.grey_red.highlights;
	const float shadows    = stretch_params.grey_red.shadows;

	const float hsRangeFactor = highlights == shadows ? 1.0f : 1.0f / (highlights - shadows);

	const T nativeShadows = shadows * maxInput;
	const T nativeHighlights = highlights * maxInput;

	const float k1 = (midtones - 1) * hsRangeFactor * maxOutput / maxInput;
	const float k2 = ((2 * midtones) - 1) * hsRangeFactor / maxInput;

	QVector<QFuture<void>> futures;
	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ?  num_threads : DEFAULT_THREADS;
	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = ceil(image_height / (double)num_threads);
		futures.append(QtConcurrent::run([ = ]() {
			int start_row = chunk * rank;
			int end_row = start_row + chunk;
			end_row = (end_row > image_height) ? image_height : end_row;
			//indigo_debug("stretchOneChannel(): %d - start_row %d, end_row %d, rows %d", rank, start_row, end_row, end_row-start_row);
			for (int j = start_row, jout = start_row; j < end_row; j += sampling, jout++) {
				T * inputLine  = input_buffer + j * image_width;
				auto * scanLine = reinterpret_cast<QRgb*>(output_image->scanLine(jout));
				QCoreApplication::processEvents();
				for (int i = 0, iout = 0; i < image_width; i += sampling, iout++) {
					const T input = inputLine[i];
					if (input < nativeShadows) output_image->setPixel(iout, jout, qRgb(0, 0, 0));
					else if (input >= nativeHighlights) output_image->setPixel(iout, jout, qRgb(maxOutput,maxOutput,maxOutput));
					else {
						const T inputFloored = (input - nativeShadows);
						int val = (inputFloored * k1) / (inputFloored * k2 - midtones);
						scanLine[iout] = qRgb(val, val, val);
					}
				}
			}
		}));
	}
	for(QFuture<void> future : futures) future.waitForFinished();
}

template <typename T>
void stretchThreeChannels(
	T *inputBuffer, QImage *outputImage,
	const StretchParams &stretchParams,
	double inputRange,
	int imageHeight,
	int imageWidth,
	int sampling
) {
	constexpr int maxOutput = 255;

	const double maxInput = inputRange > 1 ? inputRange - 1 : inputRange;

	float midtonesR   = stretchParams.grey_red.midtones;
	float highlightsR = stretchParams.grey_red.highlights;
	float shadowsR    = stretchParams.grey_red.shadows;
	float midtonesG   = stretchParams.green.midtones;
	float highlightsG = stretchParams.green.highlights;
	float shadowsG    = stretchParams.green.shadows;
	float midtonesB   = stretchParams.blue.midtones;
	float highlightsB = stretchParams.blue.highlights;
	float shadowsB    = stretchParams.blue.shadows;

	if (stretchParams.refChannel) {
		midtonesR   = stretchParams.refChannel->midtones;
		highlightsR = stretchParams.refChannel->highlights;
		shadowsR    = stretchParams.refChannel->shadows;
		midtonesG   = stretchParams.refChannel->midtones;
		highlightsG = stretchParams.refChannel->highlights;
		shadowsG    = stretchParams.refChannel->shadows;
		midtonesB   = stretchParams.refChannel->midtones;
		highlightsB = stretchParams.refChannel->highlights;
		shadowsB    = stretchParams.refChannel->shadows;
	}

	const float hsRangeFactorR = highlightsR == shadowsR ? 1.0f : 1.0f / (highlightsR - shadowsR);
	const float hsRangeFactorG = highlightsG == shadowsG ? 1.0f : 1.0f / (highlightsG - shadowsG);
	const float hsRangeFactorB = highlightsB == shadowsB ? 1.0f : 1.0f / (highlightsB - shadowsB);

	const T nativeShadowsR = shadowsR * maxInput;
	const T nativeShadowsG = shadowsG * maxInput;
	const T nativeShadowsB = shadowsB * maxInput;
	const T nativeHighlightsR = highlightsR * maxInput;
	const T nativeHighlightsG = highlightsG * maxInput;
	const T nativeHighlightsB = highlightsB * maxInput;

	const float k1R = (midtonesR - 1) * hsRangeFactorR * maxOutput / maxInput;
	const float k1G = (midtonesG - 1) * hsRangeFactorG * maxOutput / maxInput;
	const float k1B = (midtonesB - 1) * hsRangeFactorB * maxOutput / maxInput;
	const float k2R = ((2 * midtonesR) - 1) * hsRangeFactorR / maxInput;
	const float k2G = ((2 * midtonesG) - 1) * hsRangeFactorG / maxInput;
	const float k2B = ((2 * midtonesB) - 1) * hsRangeFactorB / maxInput;

	const int skip = sampling * 3;
	const int imageWidth3 = imageWidth * 3;

	QVector<QFuture<void>> futures;
	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ?  num_threads : DEFAULT_THREADS;
	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = ceil(imageHeight / (double)num_threads);
		futures.append(QtConcurrent::run([ = ]() {
			int start_row = chunk * rank;
			int end_row = start_row + chunk;
			end_row = (end_row > imageHeight) ? imageHeight : end_row;
			//indigo_error("stretchThreeChannels(): %d - start_row %d, end_row %d, rows %d", rank, start_row, end_row, end_row-start_row);
			int index = start_row * imageWidth3;
			for (int j = start_row, jout = start_row; j < end_row; j += sampling, jout++) {
				QCoreApplication::processEvents();
				auto * scanLine = reinterpret_cast<QRgb*>(outputImage->scanLine(jout));
				for (int i = 0, iout = 0; i < imageWidth3; i += skip, iout++) {
					const T inputR = inputBuffer[index++];
					const T inputG = inputBuffer[index++];
					const T inputB = inputBuffer[index++];

					uint8_t red, green, blue;

					if (inputR < nativeShadowsR) red = 0;
					else if (inputR >= nativeHighlightsR) red = maxOutput;
					else {
						const T inputFloored = (inputR - nativeShadowsR);
						red = (inputFloored * k1R) / (inputFloored * k2R - midtonesR);
					}

					if (inputG < nativeShadowsG) green = 0;
					else if (inputG >= nativeHighlightsG) green = maxOutput;
					else {
						const T inputFloored = (inputG - nativeShadowsG);
						green = (inputFloored * k1G) / (inputFloored * k2G - midtonesG);
					}

					if (inputB < nativeShadowsB) blue = 0;
					else if (inputB >= nativeHighlightsB) blue = maxOutput;
					else {
						const T inputFloored = (inputB - nativeShadowsB);
						blue = (inputFloored * k1B) / (inputFloored * k2B - midtonesB);
					}
					scanLine[iout] = qRgb(red, green, blue);
				}
			}
		}));
	}
	for(QFuture<void> future : futures) future.waitForFinished();
}

template <typename T>
void computeParamsOneChannel(
	T const *buffer,
	StretchParams1Channel *params,
	double inputRange,
	int height,
	int width,
	const float B = 0.25,
	const float C = -2.8
) {
	constexpr int maxSamples = 50000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;

	T medianSample = median(buffer, width * height, sampleBy);
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::vector<T> deviations(numSamples);
	for (int index = 0, i = 0; i < numSamples; ++i, index += sampleBy) {
		if (medianSample > buffer[index])
			deviations[i] = medianSample - buffer[index];
		else
			deviations[i] = buffer[index] - medianSample;
	}

	// scale to 0 -> 1.0.
	const float medDev = median(deviations);
	const float normalizedMedian = medianSample / inputRange;
	const float MADN = 1.4826 * medDev / inputRange;

	const bool upperHalf = normalizedMedian > 0.5;

	const float shadows = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + C * MADN)));

	const float highlights = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - C * MADN)));

	float X, M;
	if (!upperHalf) {
		X = normalizedMedian - shadows;
		M = B;
    } else {
		X = B;
		M = highlights - normalizedMedian;
	}
	float midtones;
	if (X == 0) midtones = 0.0f;
	else if (X == M) midtones = 0.5f;
	else if (X == 1) midtones = 1.0f;
	else midtones = ((M - 1) * X) / ((2 * M - 1) * X - M);

	params->shadows = shadows;
	params->highlights = highlights;
	params->midtones = midtones;
	params->shadows_expansion = 0.0;
	params->highlights_expansion = 1.0;
}

template <typename T>
void computeParamsThreeChannels(
	T const *buffer,
	StretchParams *params,
	double inputRange,
	int height,
	int width,
	const float B = 0.25,
	const float C = -2.8
) {
	constexpr int maxSamples = 50000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;
	const int sampleBy3 = sampleBy * 3;

	T medianSampleR = median(buffer, width * height * 3, sampleBy3);
	T medianSampleG = median(buffer+1, width * height * 3, sampleBy3);
	T medianSampleB = median(buffer+2, width * height * 3, sampleBy3);

	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::vector<T> deviationsR(numSamples);
	std::vector<T> deviationsG(numSamples);
	std::vector<T> deviationsB(numSamples);

	for (int index = 0, i = 0; i < numSamples; ++i, index += sampleBy3) {
		T value = buffer[index];
		if (medianSampleR > value)
			deviationsR[i] = medianSampleR - value;
		else
			deviationsR[i] = value - medianSampleR;

		value = buffer[index+1];
		if (medianSampleG > value)
			deviationsG[i] = medianSampleG - value;
		else
			deviationsG[i] = value - medianSampleG;

		value = buffer[index+2];
		if (medianSampleB > value)
			deviationsB[i] = medianSampleB - value;
		else
			deviationsB[i] = value - medianSampleB;
	}

	// scale to 0 -> 1.0.
	float medDev = median(deviationsR);
	float normalizedMedian = medianSampleR / inputRange;
	float MADN = 1.4826 * medDev / inputRange;

	bool upperHalf = normalizedMedian > 0.5;

	float shadowsR = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + C * MADN)));

	float highlightsR = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - C * MADN)));

	float X, M;
	if (!upperHalf) {
		X = normalizedMedian - shadowsR;
		M = B;
	} else {
		X = B;
		M = highlightsR - normalizedMedian;
	}
	float midtonesR;
	if (X == 0) midtonesR = 0.0f;
	else if (X == M) midtonesR = 0.5f;
	else if (X == 1) midtonesR = 1.0f;
	else midtonesR = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Green
	medDev = median(deviationsG);
	normalizedMedian = medianSampleG / inputRange;
	MADN = 1.4826 * medDev / inputRange;

	upperHalf = normalizedMedian > 0.5;

	const float shadowsG = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	const float highlightsG = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	if (!upperHalf) {
		X = normalizedMedian - shadowsG;
		M = B;
	} else {
		X = B;
		M = highlightsG - normalizedMedian;
	}
	float midtonesG;
	if (X == 0) midtonesG = 0.0f;
	else if (X == M) midtonesG = 0.5f;
	else if (X == 1) midtonesG = 1.0f;
	else midtonesG = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Blue
	medDev = median(deviationsB);
	normalizedMedian = medianSampleB / inputRange;
	MADN = 1.4826 * medDev / inputRange;

	upperHalf = normalizedMedian > 0.5;

	const float shadowsB = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	const float highlightsB = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	if (!upperHalf) {
		X = normalizedMedian - shadowsB;
		M = B;
	} else {
		X = B;
		M = highlightsB - normalizedMedian;
	}
	float midtonesB;
	if (X == 0) midtonesB = 0.0f;
	else if (X == M) midtonesB = 0.5f;
	else if (X == 1) midtonesB = 1.0f;
	else midtonesB = ((M - 1) * X) / ((2 * M - 1) * X - M);

	params->grey_red.shadows = shadowsR;
	params->grey_red.highlights = highlightsR;
	params->grey_red.midtones = midtonesR;
	params->grey_red.shadows_expansion = 0.0;
	params->grey_red.highlights_expansion = 1.0;
	params->green.shadows = shadowsG;
	params->green.highlights = highlightsG;
	params->green.midtones = midtonesG;
	params->green.shadows_expansion = 0.0;
	params->green.highlights_expansion = 1.0;
	params->blue.shadows = shadowsB;
	params->blue.highlights = highlightsB;
	params->blue.midtones = midtonesB;
	params->blue.shadows_expansion = 0.0;
	params->blue.highlights_expansion = 1.0;
}

double getRange(int data_type) {
	switch (data_type) {
		case PIX_FMT_Y8:
		case PIX_FMT_RGB24:
			return (double)0xFF;
		case PIX_FMT_Y16:
		case PIX_FMT_RGB48:
			return (double)0xFFFF;
		case PIX_FMT_Y32:
		case PIX_FMT_F32:
		case PIX_FMT_RGB96:
		case PIX_FMT_RGBF:
			return (double)0xFFFFFFFF;
		default:
			return 1.0;
	}
}

Stretcher::Stretcher(int width, int height, int data_type) {
	m_image_width = width;
	m_image_height = height;
	m_pix_fmt = data_type;
	m_input_range = getRange(m_pix_fmt);
}

void Stretcher::stretch(uint8_t const *input, QImage *outputImage, int sampling) {
	Q_ASSERT(outputImage->width() == (m_image_width + sampling - 1) / sampling);
	Q_ASSERT(outputImage->height() == (m_image_height + sampling - 1) / sampling);
	/*
	{
		indigo_raw_type rt = INDIGO_RAW_MONO8;
		double mul = 255;
		switch (m_pix_fmt) {
			case PIX_FMT_Y8:
				rt = INDIGO_RAW_MONO8;
				mul = 255;
			break;
			case PIX_FMT_Y16:
				rt = INDIGO_RAW_MONO16;
				mul = 65535;
				break;
			case PIX_FMT_RGB24:
				rt = INDIGO_RAW_RGB24;
				mul = 255;
				break;
			case PIX_FMT_RGB48:
				rt = INDIGO_RAW_RGB48;
				mul = 65535;
				break;
			default:
				break;
		}
		bool saturated = false;
		static unsigned char *mask = NULL;
		if (mask == NULL) indigo_init_saturation_mask(m_image_width, m_image_height, &mask);
		if (mask == NULL) indigo_error("alloc failed");
		double contrast = indigo_contrast(rt, input, mask, m_image_width, m_image_height, &saturated);
		if (saturated) {
			indigo_update_saturation_mask(rt, input, m_image_width, m_image_height, mask);
			indigo_raw_header *rh = (indigo_raw_header*)indigo_safe_malloc(sizeof(indigo_raw_header) + m_image_height * m_image_width);
			rh->signature = INDIGO_RAW_MONO8;
			rh->width = m_image_width;
			rh->height = m_image_height;
			memcpy((char*)rh + sizeof(indigo_raw_header), mask,  m_image_height * m_image_width);
			FILE * file= fopen("/home/rumen/mask.raw", "wb");
			if (file != nullptr) {
				indigo_error("write frame");
				fwrite(rh, sizeof(indigo_raw_header) + m_image_height * m_image_width, 1, file);
				fclose(file);
			}
			free(rh);
		}
		//free(mask);
		indigo_error("frame contrast = %f %s", contrast * mul, saturated ? "(saturated)" : "");
	}
	*/
	switch (m_pix_fmt) {
		case PIX_FMT_Y8:
			stretchOneChannel(reinterpret_cast<uint8_t const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
		break;
		case PIX_FMT_Y16:
			stretchOneChannel(reinterpret_cast<uint16_t const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
			break;
		case PIX_FMT_Y32:
			stretchOneChannel(reinterpret_cast<uint32_t const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
			break;
		case PIX_FMT_F32:
			stretchOneChannel(reinterpret_cast<float const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
			break;
		case PIX_FMT_RGB24:
			stretchThreeChannels(reinterpret_cast<uint8_t const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
			break;
		case PIX_FMT_RGB48:
			stretchThreeChannels(reinterpret_cast<uint16_t const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
			break;
		case PIX_FMT_RGB96:
			stretchThreeChannels(reinterpret_cast<uint32_t const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
		break;
		case PIX_FMT_RGBF:
			stretchThreeChannels(reinterpret_cast<float const*>(input), outputImage, m_params,
			                m_input_range, m_image_height, m_image_width, sampling);
			break;
		default:
			break;
	}
}

StretchParams Stretcher::computeParams(uint8_t const *input, const float B, const float C) {
	StretchParams result;

	StretchParams1Channel *params = &result.grey_red;
	switch (m_pix_fmt) {
		case PIX_FMT_Y8: {
			auto buffer = reinterpret_cast<uint8_t const*>(input);
			computeParamsOneChannel(buffer, params, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_Y16: {
			auto buffer = reinterpret_cast<uint16_t const*>(input);
			computeParamsOneChannel(buffer, params, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_Y32: {
			auto buffer = reinterpret_cast<uint32_t const*>(input);
			computeParamsOneChannel(buffer, params, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_F32: {
			auto buffer = reinterpret_cast<float const*>(input);
			computeParamsOneChannel(buffer, params, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_RGB24:{
			auto buffer = reinterpret_cast<uint8_t const*>(input);
			computeParamsThreeChannels(buffer, &result, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_RGB48: {
			auto buffer = reinterpret_cast<uint16_t const*>(input);
			computeParamsThreeChannels(buffer, &result, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_RGB96: {
			auto buffer = reinterpret_cast<uint32_t const*>(input);
			computeParamsThreeChannels(buffer, &result, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		case PIX_FMT_RGBF: {
			auto buffer = reinterpret_cast<float const*>(input);
			computeParamsThreeChannels(buffer, &result, m_input_range, m_image_height, m_image_width, B, C);
			break;
		}
		default:
			break;
	}
	return result;
}
