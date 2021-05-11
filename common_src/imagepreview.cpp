// Copyright (c) 2020 Rumen G.Bogdanovski
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

#include <math.h>
#include <fits.h>
#include <debayer.h>
#include <pixelformat.h>
#include <imagepreview.h>
#include <QPainter>

// Related Functions

preview_image* create_tiff_preview(unsigned char *tiff_image_buffer, unsigned long tiff_size) {
	indigo_error("PREVIEW: %s(): not implemented!", __FUNCTION__);
	preview_image* img = new preview_image();
	/* not supported yet */
	return img;
}


preview_image* create_qtsupported_preview(unsigned char *image_buffer, unsigned long size) {
	indigo_debug("PREVIEW: %s(): called", __FUNCTION__);
	preview_image* img = new preview_image();
	img->loadFromData((const uchar*)image_buffer, size);
	if(img->width() == 0 || img->height() == 0) {
		delete img;
		return nullptr;
	}
	return img;
}


preview_image* create_jpeg_preview(unsigned char *jpg_buffer, unsigned long jpg_size) {
#if !defined(USE_LIBJPEG)

	preview_image* img = new preview_image();
	img->loadFromData((const uchar*)jpg_buffer, jpg_size, "JPG");
	return img;

#else // INDIGO Mac and Linux

	unsigned char *bmp_buffer;
	unsigned long bmp_size;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int row_stride, width, height, pixel_size, color_space;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);

	int rc = jpeg_read_header(&cinfo, TRUE);
	if (rc != 1) {
		indigo_error("JPEG: Data does not seem to be JPEG");
		return nullptr;
	}
	jpeg_start_decompress(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;
	color_space = cinfo.out_color_space;
	indigo_debug("JPEG: Image is %d x %d (BPP: %d CS: %d)", width, height, pixel_size*8, color_space);

	bmp_size = width * height * pixel_size;
	bmp_buffer = (unsigned char*)malloc(bmp_size);
	row_stride = width * pixel_size;

	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	preview_image* img;
	if (color_space == JCS_GRAYSCALE) {
		img = new preview_image(width, height, QImage::Format_Indexed8);
	} else if (color_space == JCS_RGB) {
		img = new preview_image(width, height, QImage::Format_RGB888);
	} else {
		indigo_error("JPEG: Unsupported colour space (CS: %d)", color_space);
		return nullptr;
	}

	for (int y = 0; y < img->height(); y++) {
		memcpy(img->scanLine(y), bmp_buffer + y * row_stride, row_stride);
	}

	free(bmp_buffer);
	return img;
#endif
}


preview_image* create_fits_preview(unsigned char *raw_fits_buffer, unsigned long fits_size, const double white_threshold) {
	fits_header header;
	int *hist;
	unsigned int pix_format = 0;

	int res = fits_read_header(raw_fits_buffer, fits_size, &header);
	if (res != FITS_OK) {
		indigo_error("FITS: Error parsing header");
		return nullptr;
	}

	if ((header.bitpix==16) && (header.naxis == 2)){
		hist = (int*)malloc(65536*sizeof(int));
		pix_format = PIX_FMT_Y16;
	} else if ((header.bitpix==16) && (header.naxis == 3)){
		hist = (int*)malloc(65536*sizeof(int));
		pix_format = PIX_FMT_3RGB48;
	} else if ((header.bitpix==8) && (header.naxis == 2)){
		hist = (int*)malloc(256*sizeof(int));
		pix_format = PIX_FMT_Y8;
	} else if ((header.bitpix==8) && (header.naxis == 3)){
		hist = (int*)malloc(256*sizeof(int));
		pix_format = PIX_FMT_3RGB24;
	} else {
		indigo_error("FITS: Unsupported bitpix (BITPIX= %d)", header.bitpix);
		return nullptr;
	}

	char *fits_data = (char*)malloc(fits_get_buffer_size(&header));

	res = fits_process_data_with_hist(raw_fits_buffer, fits_size, &header, fits_data, hist);
	if (res != FITS_OK) {
		indigo_error("FITS: Error processing data");
		return nullptr;
	}

	if (header.naxis == 2) {
		if (!strcmp(header.bayerpat, "BGGR") && (header.bitpix == 8)) {
			pix_format = PIX_FMT_SBGGR8;
		} else if (!strcmp(header.bayerpat, "GBRG") && (header.bitpix == 8)) {
			pix_format = PIX_FMT_SGBRG8;
		} else if (!strcmp(header.bayerpat, "GRBG") && (header.bitpix == 8)) {
			pix_format = PIX_FMT_SGRBG8;
		} else if (!strcmp(header.bayerpat, "RGGB") && (header.bitpix == 8)) {
			pix_format = PIX_FMT_SRGGB8;
		} else if (!strcmp(header.bayerpat, "BGGR") && (header.bitpix == 16)) {
			pix_format = PIX_FMT_SBGGR16;
		} else if (!strcmp(header.bayerpat, "GBRG") && (header.bitpix == 16)) {
			pix_format = PIX_FMT_SGBRG16;
		} else if (!strcmp(header.bayerpat, "GRBG") && (header.bitpix == 16)) {
			pix_format = PIX_FMT_SGRBG16;
		} else if (!strcmp(header.bayerpat, "RGGB") && (header.bitpix == 16)) {
			pix_format = PIX_FMT_SRGGB16;
		}
	}

	preview_image *img = create_preview(header.naxisn[0], header.naxisn[1],
	        pix_format, fits_data, hist, white_threshold);

	free(fits_data);
	return img;
}


preview_image* create_raw_preview(unsigned char *raw_image_buffer, unsigned long raw_size, const double white_threshold) {
	int *hist;
	unsigned int pix_format;
	int bitpix;

	if (sizeof(indigo_raw_header) > raw_size) {
		indigo_error("RAW: Image buffer is too short: can not fit the header (%dB)", raw_size);
		return nullptr;
	}

	indigo_raw_header *header = (indigo_raw_header*)raw_image_buffer;
	char *raw_data = (char*)raw_image_buffer + sizeof(indigo_raw_header);

	int pixel_count = header->height * header->width;

	switch (header->signature) {
	case INDIGO_RAW_MONO16:
		pix_format = PIX_FMT_Y16;
		bitpix = 16;
		break;
	case INDIGO_RAW_MONO8:
		pix_format = PIX_FMT_Y8;
		bitpix = 8;
		break;
	case INDIGO_RAW_RGB24:
		pix_format = PIX_FMT_RGB24;
		bitpix = 8;
		break;
	case INDIGO_RAW_RGB48:
		pix_format = PIX_FMT_RGB48;
		bitpix = 16;
		break;
	default:
		indigo_error("RAW: Unsupported image format (%d)", header->signature);
		return nullptr;
	}

	if ((pixel_count * bitpix / 8 + sizeof(indigo_raw_header)) > raw_size) {
		indigo_error("RAW: Image buffer is too short: can not fit the image (%dB)", raw_size);
		return nullptr;
	}

	if ((header->signature == INDIGO_RAW_MONO16) ||
	    (header->signature == INDIGO_RAW_RGB48)) {
		hist = (int*)malloc(65536 * sizeof(int));
		memset(hist, 0, 65536 * sizeof(int));
		uint16_t* buf = (uint16_t*)raw_data;
		for (int pix = 0; pix < pixel_count; ++pix) {
			hist[*buf++]++;
		}
	} else if ((header->signature == INDIGO_RAW_MONO8) ||
	           (header->signature == INDIGO_RAW_RGB24)) {
		hist = (int*)malloc(256*sizeof(int));
		memset(hist, 0, 256*sizeof(int));
		uint8_t* buf = (uint8_t*)raw_data;
		for (int pix = 0; pix < pixel_count; ++pix) {
			hist[*buf++]++;
		}
	} else {
		// should not happen - handled above
		return nullptr;
	}

	preview_image *img = create_preview(header->width, header->height,
	        pix_format, raw_data, hist, white_threshold);

	return img;
}


preview_image* create_preview(int width, int height, int pix_format, char *image_data, int *hist, double white_threshold) {
	int range, max, min = 0, sum;
	int pix_cnt = width * height;
	int thresh = (int)(white_threshold / 100.0 * pix_cnt); // white thresh is in percentiles

	switch (pix_format) {
	case PIX_FMT_Y8:
	case PIX_FMT_RGB24:
	case PIX_FMT_3RGB24:
	case PIX_FMT_SBGGR8:
	case PIX_FMT_SGBRG8:
	case PIX_FMT_SGRBG8:
	case PIX_FMT_SRGGB8:
		max = 255;
		break;
	case PIX_FMT_Y16:
	case PIX_FMT_RGB48:
	case PIX_FMT_3RGB48:
	case PIX_FMT_SBGGR16:
	case PIX_FMT_SGBRG16:
	case PIX_FMT_SGRBG16:
	case PIX_FMT_SRGGB16:
		max = 65535;
		break;
	default:
		indigo_error("PREVIEW: Unsupported pixel format (%d)", pix_format);
		return nullptr;
	}

	min = 0;
	while (hist[min] == 0 && min < max) {
		min++;
	};

	sum = hist[max];
	while (sum < thresh && max > min) {
		sum += hist[--max];
	}

	switch (pix_format) {
	case PIX_FMT_Y8:
	case PIX_FMT_RGB24:
	case PIX_FMT_3RGB24:
	case PIX_FMT_SBGGR8:
	case PIX_FMT_SGBRG8:
	case PIX_FMT_SGRBG8:
	case PIX_FMT_SRGGB8:
		if (fabs(max - min) < 2) {
			if (min >= 2) {
				min -= 2;
			} else if (max <= 253) {
				max += 2;
			}
		}
		break;
	case PIX_FMT_Y16:
	case PIX_FMT_RGB48:
	case PIX_FMT_3RGB48:
	case PIX_FMT_SBGGR16:
	case PIX_FMT_SGBRG16:
	case PIX_FMT_SGRBG16:
	case PIX_FMT_SRGGB16:
		if (fabs(max - min) < 2) {
			if (min >= 2) {
				min -= 2;
			} else if (max <= 65533) {
				max += 2;
			}
		}
		break;
	default:
		indigo_error("PREVIEW: Unsupported pixel format (%d)", pix_format);
		return nullptr;
	}

	range = max - min;
	if (range < 2) range = 2;
	double scale = 256.0 / range;

	indigo_debug("PREVIEW: pix_format = %d sum = %d thresh = %d max = %d min = %d", pix_format, sum, thresh, max, min);

	preview_image* img = new preview_image(width, height, QImage::Format_RGB888);
	if (pix_format == PIX_FMT_Y8) {
		uint8_t* buf = (uint8_t*)image_data;
		uint8_t* pixmap_data = (uint8_t*)malloc(sizeof(uint8_t) * height * width);
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pixmap_data[index] = buf[index];
				int value = buf[index++] - min;
				if (value >= range) value = 255;
				else value *= scale;
				img->setPixel(x, y, qRgb(value, value, value));
			}
		}
		img->m_raw_data = (char*)pixmap_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_Y8;
		img->m_height = height;
		img->m_width = width;
	} else if (pix_format == PIX_FMT_Y16) {
		uint16_t* buf = (uint16_t*)image_data;
		uint16_t* pixmap_data = (uint16_t*)malloc(sizeof(uint16_t) * height * width);
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pixmap_data[index] = buf[index];
				int value = buf[index++] - min;
				if (value >= range) value = 255;
				else value *= scale;
				img->setPixel(x, y, qRgb(value, value, value));
			}
		}
		img->m_raw_data = (char*)pixmap_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_Y16;
		img->m_height = height;
		img->m_width = width;
	} else if (pix_format == PIX_FMT_3RGB24) {
		int channel_offest = width * height;
		uint8_t* buf = (uint8_t*)image_data;
		uint8_t* pixmap_data = (uint8_t*)malloc(sizeof(uint8_t) * height * width * 3);
		int index = 0;
		int index2 = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value_r = buf[index] - min;
				int value_g = buf[index + channel_offest] - min;
				int value_b = buf[index + 2 * channel_offest] - min;
				pixmap_data[index2 + 0] = buf[index];
				pixmap_data[index2 + 1] = buf[index + channel_offest];
				pixmap_data[index2 + 2] = buf[index + 2 * channel_offest];
				index++;
				index2 += 3;
				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
		img->m_raw_data = (char*)pixmap_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_RGB24;
		img->m_height = height;
		img->m_width = width;
	} else if (pix_format == PIX_FMT_3RGB48) {
		int channel_offest = width * height;
		uint16_t* buf = (uint16_t*)image_data;
		uint16_t* pixmap_data = (uint16_t*)malloc(sizeof(uint16_t) * height * width * 3);
		int index = 0;
		int index2 = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value_r = buf[index] - min;
				int value_g = buf[index + channel_offest] - min;
				int value_b = buf[index + 2 * channel_offest] - min;
				pixmap_data[index2 + 0] = buf[index];
				pixmap_data[index2 + 1] = buf[index + channel_offest];
				pixmap_data[index2 + 2] = buf[index + 2 * channel_offest];
				index++;
				index2 += 3;
				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
		img->m_raw_data = (char*)pixmap_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_RGB48;
		img->m_height = height;
		img->m_width = width;
	} else if (pix_format == PIX_FMT_RGB24) {
		uint8_t* buf = (uint8_t*)image_data;
		uint8_t* pixmap_data = (uint8_t*)malloc(sizeof(uint8_t) * height * width * 3);
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pixmap_data[index] = buf[index];
				int value_r = buf[index] - min;
				index++;
				pixmap_data[index] = buf[index];
				int value_g = buf[index] - min;
				index++;
				pixmap_data[index] = buf[index];
				int value_b = buf[index] - min;
				index++;

				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
		img->m_raw_data = (char*)pixmap_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_RGB24;
		img->m_height = height;
		img->m_width = width;
	} else if (pix_format == PIX_FMT_RGB48) {
		uint16_t* buf = (uint16_t*)image_data;
		uint16_t* pixmap_data = (uint16_t*)malloc(sizeof(uint16_t) * height * width * 3);
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pixmap_data[index] = buf[index];
				int value_r = buf[index] - min;
				index++;
				pixmap_data[index] = buf[index];
				int value_g = buf[index] - min;
				index++;
				pixmap_data[index] = buf[index];
				int value_b = buf[index] - min;
				index++;

				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
		img->m_raw_data = (char*)pixmap_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_RGB48;
		img->m_height = height;
		img->m_width = width;
	} else if ((pix_format == PIX_FMT_SBGGR8) || (pix_format == PIX_FMT_SGBRG8) ||
		       (pix_format == PIX_FMT_SGRBG8) || (pix_format == PIX_FMT_SRGGB8)) {
		uint8_t* rgb_data = (uint8_t*)malloc(width*height*3);
		bayer_to_rgb24((unsigned char*)image_data, rgb_data, width, height, pix_format);
		uint8_t* buf = (uint8_t*)rgb_data;
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value_r = buf[index++] - min;
				int value_g = buf[index++] - min;
				int value_b = buf[index++] - min;

				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
		img->m_raw_data = (char*)rgb_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_RGB24;
		img->m_height = height;
		img->m_width = width;
	} else if ((pix_format == PIX_FMT_SBGGR16) || (pix_format == PIX_FMT_SGBRG16) ||
		       (pix_format == PIX_FMT_SGRBG16) || (pix_format == PIX_FMT_SRGGB16)) {
		uint16_t* rgb_data = (uint16_t*)malloc(width*height*6);
		bayer_to_rgb48((const uint16_t*)image_data, rgb_data, width, height, pix_format);
		uint16_t* buf = (uint16_t*)rgb_data;
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value_r = buf[index++] - min;
				int value_g = buf[index++] - min;
				int value_b = buf[index++] - min;

				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
		img->m_raw_data = (char*)rgb_data;
		img->m_histogram = hist;
		img->m_pix_format = PIX_FMT_RGB48;
		img->m_height = height;
		img->m_width = width;
	} else {
		indigo_error("PREVIEW: Unsupported pixel format (%d)", pix_format);
		delete img;
		img = nullptr;
	}
	return img;
}


void stretch_preview(preview_image *img, double white_threshold) {
	int range, max, min = 0, sum;
	int width = img->m_width;
	int height = img->m_height;
	int pix_format = img->m_pix_format;
	int *hist = img->m_histogram;

	if ((hist == nullptr) || (img->m_raw_data == nullptr)) {
		indigo_debug("image not scalable");
		return;
	}

	int pix_cnt = width * height;
	int thresh = (int)(white_threshold / 100.0 * pix_cnt); // white thresh is in percentiles

	switch (pix_format) {
	case PIX_FMT_Y8:
	case PIX_FMT_RGB24:
		max = 255;
		break;
	case PIX_FMT_Y16:
	case PIX_FMT_RGB48:
		max = 65535;
		break;
	default:
		indigo_error("PREVIEW: Unsupported pixel format (%d)", pix_format);
		return;
	}

	min = 0;
	while (hist[min] == 0 && min < max) {
		min++;
	};

	sum = hist[max];
	while (sum < thresh && max > min) {
		sum += hist[--max];
	}

	switch (pix_format) {
	case PIX_FMT_Y8:
	case PIX_FMT_RGB24:
		if (fabs(max - min) < 2) {
			if (min >= 2) {
				min -= 2;
			} else if (max <= 253) {
				max += 2;
			}
		}
		break;
	case PIX_FMT_Y16:
	case PIX_FMT_RGB48:
		if (fabs(max - min) < 2) {
			if (min >= 2) {
				min -= 2;
			} else if (max <= 65533) {
				max += 2;
			}
		}
		break;
	default:
		indigo_error("PREVIEW: Unsupported pixel format (%d)", pix_format);
		return;
	}

	range = max - min;
	if (range < 2) range = 2;
	double scale = 256.0 / range;

	indigo_debug("PREVIEW: pix_format = %d sum = %d thresh = %d max = %d min = %d", pix_format, sum, thresh, max, min);

	if (pix_format == PIX_FMT_Y8) {
		uint8_t* buf = (uint8_t*)img->m_raw_data;
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value = buf[index++] - min;
				if (value >= range) value = 255;
				else value *= scale;
				img->setPixel(x, y, qRgb(value, value, value));
			}
		}
	} else if (pix_format == PIX_FMT_Y16) {
		uint16_t* buf = (uint16_t*)img->m_raw_data;
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value = buf[index++] - min;
				if (value >= range) value = 255;
				else value *= scale;
				img->setPixel(x, y, qRgb(value, value, value));
			}
		}
	} else if (pix_format == PIX_FMT_RGB24) {
		uint8_t* buf = (uint8_t*)img->m_raw_data;
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value_r = buf[index] - min;
				index++;
				int value_g = buf[index] - min;
				index++;
				int value_b = buf[index] - min;
				index++;

				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
	} else if (pix_format == PIX_FMT_RGB48) {
		uint16_t* buf = (uint16_t*)img->m_raw_data;
		int index = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int value_r = buf[index] - min;
				index++;
				int value_g = buf[index] - min;
				index++;
				int value_b = buf[index] - min;
				index++;

				if (value_r >= range) value_r = 255;
				else value_r *= scale;
				if (value_g >= range) value_g = 255;
				else value_g *= scale;
				if (value_b >= range) value_b = 255;
				else value_b *= scale;
				img->setPixel(x, y, qRgb(value_r, value_g, value_b));
			}
		}
	} else {
		indigo_error("PREVIEW: Unsupported pixel format (%d)", pix_format);
	}
}

preview_image* create_preview(indigo_property *property, indigo_item *item, const double white_threshold) {
	preview_image *preview = nullptr;
	if (property->type == INDIGO_BLOB_VECTOR && property->state == INDIGO_OK_STATE) {
		preview = create_preview(item, white_threshold);
	}
	return preview;
}

preview_image* create_preview(indigo_item *item, const double white_threshold) {
	return create_preview((unsigned char*)item->blob.value, item->blob.size, item->blob.format, white_threshold);
}

preview_image* create_preview(unsigned char *data, size_t size, const char* format, const double white_threshold) {
	preview_image *preview = nullptr;
	if (data != NULL && format != NULL) {
		if (!strcmp(format, ".jpeg") ||
			!strcmp(format, ".jpg")  ||
			!strcmp(format, ".JPG")  ||
			!strcmp(format, ".JPEG")
		) {
			preview = create_jpeg_preview(data, size);
		} else if (
			!strcmp(format, ".fits") ||
			!strcmp(format, ".fit")  ||
			!strcmp(format, ".fts")  ||
			!strcmp(format, ".FITS") ||
			!strcmp(format, ".FIT")  ||
			!strcmp(format, ".FTS")
		) {
			preview = create_fits_preview(data, size, white_threshold);
		} else if (
			!strcmp(format, ".raw") ||
			!strcmp(format, ".RAW")
		) {
			preview = create_raw_preview(data, size, white_threshold);
		} else if (
			!strcmp(format, ".tif")  ||
			!strcmp(format, ".tiff") ||
			!strcmp(format, ".TIF")  ||
			!strcmp(format, ".TIFF")
		) {
			preview = create_tiff_preview(data, size);
		} else if (format[0] != '\0') {
			/* DUMMY TEST CODE */
			/*
			FILE *file;
			char *buffer;
			unsigned long fileLen;
			char name[100] = "/home/rumen/test.png";
			file = fopen(name, "rb");
			fseek(file, 0, SEEK_END);
			fileLen=ftell(file);
			fseek(file, 0, SEEK_SET);
			buffer=(char *)malloc(fileLen+1);
			fread(buffer, fileLen, 1, file);
			fclose(file);
			preview = create_qt_preview((unsigned char*)buffer, fileLen);
			*/
			preview = create_qtsupported_preview(data, size);
		}
	}
	return preview;
}
