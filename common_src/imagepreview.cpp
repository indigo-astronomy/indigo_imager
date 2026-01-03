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

#include <setjmp.h>
#include <math.h>
#include <fits.h>
#include <xisf.h>
#include <pixelformat.h>
#include <imagepreview.h>
#include <QPainter>
#include <QCoreApplication>
#include <image_preview_lut.h>
#include <dslr_raw.h>
#include <utils.h>

#include <unistd.h>
#include <thread>
#include <mutex>

#define MIN_SIZE_TO_PARALLELIZE 0x3FFFF

// Related Functions

static std::recursive_mutex g_preview_mutex;

static void qimage_cleanup(void *info) {
	if (info) free(info);
}

int get_bayer_offsets(uint32_t pix_format) {
	switch (pix_format) {
		case PIX_FMT_SBGGR8:
		case PIX_FMT_SBGGR12:
		case PIX_FMT_SBGGR16:
		case PIX_FMT_SBGGR32:
		case PIX_FMT_SBGGRF:
			return 0x11;

		case PIX_FMT_SGBRG8:
		case PIX_FMT_SGBRG12:
		case PIX_FMT_SGBRG16:
		case PIX_FMT_SGBRG32:
		case PIX_FMT_SGBRGF:
			return 0x01;

		case PIX_FMT_SGRBG8:
		case PIX_FMT_SGRBG12:
		case PIX_FMT_SGRBG16:
		case PIX_FMT_SGRBG32:
		case PIX_FMT_SGRBGF:
			return 0x10;

		case PIX_FMT_SRGGB8:
		case PIX_FMT_SRGGB12:
		case PIX_FMT_SRGGB16:
		case PIX_FMT_SRGGB32:
		case PIX_FMT_SRGGBF:
			return 0x00;
	}
}

template <typename T> static inline void debayer(T *raw, int index, int row, int column, int width, int height, int offsets, float &red, float &green, float &blue) {
	switch (offsets ^ ((column & 1) << 4 | (row & 1))) {
		case 0x00:
			red = raw[index];
			if (column == 0) {
				if (row == 0) {
					green = (raw[index + 1] + raw[index + width]) / 2.0;
					blue = raw[index + width + 1];
				} else if (row == height - 1) {
					green = (raw[index + 1] + raw[index - width]) / 2.0;
					blue = raw[index - width + 1];
				} else {
					green = (raw[index + 1] + raw[index + width] + raw[index - width]) / 3.0;
					blue = (raw[index - width + 1] + raw[index + width + 1]) / 2.0;
				}
			} else if (column == width - 1) {
				if (row == 0) {
					green = (raw[index - 1] + raw[index + width]) / 2.0;
					blue = (raw[index + width - 1] + raw[index + width + 1]) / 2.0;
				} else if (row == height - 1) {
					green = (raw[index - 1] + raw[index - width]) / 2.0;
					blue = (raw[index - width - 1] + raw[index - width + 1]) / 2.0;
				} else {
					green = (raw[index - 1] + raw[index + width] + raw[index - width]) / 3.0;
					blue = (raw[index - width - 1] + raw[index + width - 1]) / 2.0;
				}
			} else {
				if (row == 0) {
					green = (raw[index + 1] + raw[index - 1] + raw[index + width]) / 3.0;
					blue = (raw[index + width - 1] + raw[index + width + 1]) / 2.0;
				} else if (row == height - 1) {
					green = (raw[index + 1] + raw[index - 1] + raw[index - width]) / 3.0;
					blue = (raw[index - width - 1] + raw[index - width + 1]) / 2.0;
				} else {
					green = (raw[index + 1] + raw[index - 1] + raw[index + width] + raw[index - width]) / 4.0;
					blue = (raw[index - width - 1] + raw[index - width + 1] + raw[index + width - 1] + raw[index + width + 1]) / 4.0;
				}
			}
			break;
		case 0x10:
			if (column == 0) {
				red = raw[index + 1];
			} else if (column == width - 1) {
				red = raw[index - 1];
			} else {
				red = (raw[index - 1] + raw[index + 1]) / 2.0;
			}
			green = raw[index];
			if (row == 0) {
				blue = raw[index + width];
			} else if (row == height - 1) {
				blue = raw[index - width];
			} else {
				blue = (raw[index - width] + raw[index + width]) / 2.0;
			}
			break;
		case 0x01:
			if (row == 0) {
				red = raw[index + width];
			} else if (row == height - 1) {
				red = raw[index - width];
			} else {
				red = (raw[index - width] + raw[index + width]) / 2.0;
			}
			green = raw[index];
			if (column == 0) {
				blue = raw[index + 1];
			} else if (column == width - 1) {
				blue = raw[index - 1];
			} else {
				blue = (raw[index - 1] + raw[index + 1]) / 2.0;
			}
			break;
		case 0x11:
			if (column == 0) {
				if (row == 0) {
					red = raw[index + width + 1];
					green = (raw[index + 1] + raw[index + width]) / 2.0;
				} else if (row == height - 1) {
					red = raw[index - width + 1];
					green = (raw[index + 1] + raw[index - width]) / 2.0;
				} else {
					red = raw[index - width + 1];
					green = (raw[index + 1] + raw[index + width] + raw[index - width]) / 3.0;
				}
			} else if (column == width - 1) {
				if (row == 0) {
					red = raw[index + width - 1];
					green = (raw[index - 1] + raw[index + width]) / 2.0;
				} else if (row == height - 1) {
					red = raw[index - width - 1];
					green = (raw[index - 1] + raw[index - width]) / 2.0;
				} else {
					red = (raw[index - width - 1] + raw[index + width - 1]) / 2.0;
					green = (raw[index - 1] + raw[index + width] + raw[index - width]) / 3.0;
				}
			} else {
				if (row == 0) {
					red = (raw[index + width - 1] + raw[index + width + 1]) / 2.0;
					green = (raw[index + 1] + raw[index - 1] + raw[index + width]) / 3.0;
				} else if (row == height - 1) {
					red = (raw[index - width - 1] + raw[index - width + 1]) / 2.0;
					green = (raw[index + 1] + raw[index - 1] + raw[index - width]) / 3.0;
				} else {
					red = (raw[index - width - 1] + raw[index - width + 1] + raw[index + width - 1] + raw[index + width + 1]) / 4.0;
					green = (raw[index + 1] + raw[index - 1] + raw[index + width] + raw[index - width]) / 4.0;
				}
			}
			blue = raw[index];
			break;
	}
}

template <typename T> void parallel_debayer(T *input_buffer, int width, int height, int offsets, T *output_buffer) {
	const int size = width * height;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		int input_index = 0;
		for (int row_index = 0; row_index < height; row_index++) {
			for (int column_index = 0; column_index < width; column_index++) {
				float red = 0, green = 0, blue = 0;
				int output_index = input_index * 3;
				debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
				output_buffer[output_index] = red;
				output_buffer[output_index + 1] = green;
				output_buffer[output_index + 2] = blue;
				input_index++;
			}
		}
	} else {
		int max_threads = get_number_of_cores();
		max_threads = (max_threads > 0) ? max_threads : AIN_DEFAULT_THREADS;
		std::thread threads[max_threads];
		for (int rank = 0; rank < max_threads; rank++) {
			const int chunk = ceil(height / (double)max_threads);
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				end = (end > height) ? height : end;
				int input_index = start * width;
				for (int row_index = start; row_index < end; row_index++) {
					for (int column_index = 0; column_index < width; column_index++) {
						float red = 0, green = 0, blue = 0;
						int output_index = input_index * 3;
						debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
						output_buffer[output_index] = red;
						output_buffer[output_index + 1] = green;
						output_buffer[output_index + 2] = blue;
						input_index++;
					}
				}
			});
		}
		for (int rank = 0; rank < max_threads; rank++) {
			threads[rank].join();
		}
	}
}

static unsigned int bayer_to_pix_format(const char *image_bayer_pat, const char bitpix, uint32_t prefered_bayer_pat) {
	char bayerpat[5] = {0};

	if (prefered_bayer_pat == BAYER_PAT_AUTO || prefered_bayer_pat == 0) {
		memcpy(bayerpat, image_bayer_pat, 4);
	} else {
		memcpy(bayerpat, &prefered_bayer_pat, 4);
	}
	bayerpat[4] = '\0';

	if ((!strcmp(bayerpat, "BGGR") || !strcmp(bayerpat, "BGRG")) && (bitpix == 8)) {
		return PIX_FMT_SBGGR8;
	} else if ((!strcmp(bayerpat, "GBRG") || !strcmp(bayerpat, "GBGR")) && (bitpix == 8)) {
		return PIX_FMT_SGBRG8;
	} else if ((!strcmp(bayerpat, "GRBG") || !strcmp(bayerpat, "GRGB")) && (bitpix == 8)) {
		return PIX_FMT_SGRBG8;
	} else if ((!strcmp(bayerpat, "RGGB") || !strcmp(bayerpat, "RGBG")) && (bitpix == 8)) {
		return PIX_FMT_SRGGB8;
	} else if ((!strcmp(bayerpat, "BGGR") || !strcmp(bayerpat, "BGRG")) && (bitpix == 16)) {
		return PIX_FMT_SBGGR16;
	} else if ((!strcmp(bayerpat, "GBRG") || !strcmp(bayerpat, "GBGR")) && (bitpix == 16)) {
		return PIX_FMT_SGBRG16;
	} else if ((!strcmp(bayerpat, "GRBG") || !strcmp(bayerpat, "GRGB")) && (bitpix == 16)) {
		return PIX_FMT_SGRBG16;
	} else if ((!strcmp(bayerpat, "RGGB") || !strcmp(bayerpat, "RGBG")) && (bitpix == 16)) {
		return PIX_FMT_SRGGB16;
	} else if ((!strcmp(bayerpat, "BGGR") || !strcmp(bayerpat, "BGRG")) && (bitpix == 32)) {
		return PIX_FMT_SBGGR32;
	} else if ((!strcmp(bayerpat, "GBRG") || !strcmp(bayerpat, "GBGR")) && (bitpix == 32)) {
		return PIX_FMT_SGBRG32;
	} else if ((!strcmp(bayerpat, "GRBG") || !strcmp(bayerpat, "GRGB")) && (bitpix == 32)) {
		return PIX_FMT_SGRBG32;
	} else if ((!strcmp(bayerpat, "RGGB") || !strcmp(bayerpat, "RGBG")) && (bitpix == 32)) {
		return PIX_FMT_SRGGB32;
	} else if ((!strcmp(bayerpat, "BGGR") || !strcmp(bayerpat, "BGRG")) && (bitpix == -32)) {
		return PIX_FMT_SBGGRF;
	} else if ((!strcmp(bayerpat, "GBRG") || !strcmp(bayerpat, "GBGR")) && (bitpix == -32)) {
		return PIX_FMT_SGBRGF;
	} else if ((!strcmp(bayerpat, "GRBG") || !strcmp(bayerpat, "GRGB")) && (bitpix == -32)) {
		return PIX_FMT_SGRBGF;
	} else if ((!strcmp(bayerpat, "RGGB") || !strcmp(bayerpat, "RGBG")) && (bitpix == -32)) {
		return PIX_FMT_SRGGBF;
	}
	return 0;
}

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


#if defined(USE_LIBJPEG)
static jmp_buf jpeg_error;
static void jpeg_error_cb(j_common_ptr cinfo) {
	Q_UNUSED(cinfo);
	longjmp(jpeg_error, 1);
}
#endif

preview_image* create_jpeg_preview(unsigned char *jpg_buffer, unsigned long jpg_size) {
#if !defined(USE_LIBJPEG)

	preview_image* img = new preview_image();
	img->loadFromData((const uchar*)jpg_buffer, jpg_size, "JPG");
	return img;

#else // INDIGO Mac and Linux

	unsigned char *bmp_buffer = nullptr;
	unsigned long bmp_size;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int row_stride, width, height, pixel_size, color_space;

	cinfo.err = jpeg_std_error(&jerr);
	/* override default exit() and return 2 lines below */
	jerr.error_exit = jpeg_error_cb;
	/* Jump here in case of a decmpression error */
	if (setjmp(jpeg_error)) {
		if (bmp_buffer) free(bmp_buffer);
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}

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


preview_image* create_dslr_raw_preview(unsigned char *raw_buffer, unsigned long raw_size, const stretch_config_t sconfig) {
	dslr_raw_image_s outout_image;
	unsigned int pix_format = 0;

	int rc = dslr_raw_process_image((void *)raw_buffer, raw_size, &outout_image);
	if (rc != LIBRAW_SUCCESS) {
		if (outout_image.data != nullptr) free(outout_image.data);
		return nullptr;
	}

	if (outout_image.debayered) {
		if (outout_image.bits == 8) {
			pix_format = PIX_FMT_RGB24;
		} else {
			pix_format = PIX_FMT_RGB48;
		}
	} else {
		int bayer_pix_fmt = bayer_to_pix_format(outout_image.bayer_pattern, outout_image.bits, sconfig.bayer_pattern);
		if (bayer_pix_fmt != 0) pix_format = bayer_pix_fmt;

	}

	// transfer ownership of the debayered/raw buffer to the preview to avoid extra copy
	std::shared_ptr<char> owner((char*)outout_image.data, [](char *p){ free(p); });
	preview_image *img = create_preview(outout_image.width, outout_image.height, pix_format, owner, (char*)outout_image.data, sconfig);
	return img;
}


preview_image* create_fits_preview(unsigned char *raw_fits_buffer, unsigned long fits_size, const stretch_config_t sconfig) {
	fits_header header;
	unsigned int pix_format = 0;

	indigo_debug("FITS_START");
	int res = fits_read_header(raw_fits_buffer, fits_size, &header);
	if (res != FITS_OK) {
		indigo_error("FITS: Error parsing header");
		return nullptr;
	}

	if ((header.bitpix == -32) && (header.naxis == 2)){
		pix_format = PIX_FMT_F32;
	} else if ((header.bitpix == 32) && (header.naxis == 2)){
		pix_format = PIX_FMT_Y32;
	} else if ((header.bitpix == 16) && (header.naxis == 2)){
		pix_format = PIX_FMT_Y16;
	} else if ((header.bitpix == 8) && (header.naxis == 2)){
		pix_format = PIX_FMT_Y8;
	} else if ((header.bitpix == -32) && (header.naxis == 3)){
		pix_format = PIX_FMT_3RGBF;
	} else if ((header.bitpix == 32) && (header.naxis == 3)){
		pix_format = PIX_FMT_3RGB96;
	} else if ((header.bitpix == 16) && (header.naxis == 3)){
		pix_format = PIX_FMT_3RGB48;
	} else if ((header.bitpix == 8) && (header.naxis == 3)){
		pix_format = PIX_FMT_3RGB24;
	} else {
		indigo_error("FITS: Unsupported bitpix (BITPIX= %d)", header.bitpix);
		return nullptr;
	}

	char *fits_data = (char*)malloc(fits_get_buffer_size(&header));

	res = fits_process_data(raw_fits_buffer, fits_size, &header, fits_data);
	if (res != FITS_OK) {
		indigo_error("FITS: Error processing data");
		free(fits_data);
		return nullptr;
	}

	if (header.naxis == 2) {
		int bayer_pix_fmt = bayer_to_pix_format(header.bayerpat, header.bitpix, sconfig.bayer_pattern);
		if (bayer_pix_fmt != 0) pix_format = bayer_pix_fmt;
	}

	// pass ownership of fits_data to preview to avoid an extra memcpy/free
	std::shared_ptr<char> owner(fits_data, [](char *p){ free(p); });
	preview_image *img = create_preview(header.naxisn[0], header.naxisn[1], pix_format, owner, fits_data, sconfig);

	indigo_debug("FITS_END: fits_data = %p (owned)", fits_data);
	return img;
}

preview_image* create_xisf_preview(unsigned char *xisf_buffer, unsigned long xisf_size, const stretch_config_t sconfig) {
	xisf_metadata header;
	unsigned int pix_format = 0;
	preview_image *img = nullptr;

	indigo_debug("XISF_START");
	int res = xisf_read_metadata(xisf_buffer, xisf_size, &header);
	if (res != XISF_OK) {
		indigo_error("XISF: Error parsing header res=%d", res);
		return nullptr;
	}

	if ((header.bitpix == -32) && (header.channels == 1)){
		pix_format = PIX_FMT_F32;
	} else if ((header.bitpix == 32) && (header.channels == 1)){
		pix_format = PIX_FMT_Y32;
	} else if ((header.bitpix == 16) && (header.channels == 1)){
		pix_format = PIX_FMT_Y16;
	} else if ((header.bitpix == 8) && (header.channels == 1)){
		pix_format = PIX_FMT_Y8;
	} else if ((header.bitpix == -32) && (header.channels == 3)){
		pix_format = (header.normal_pixel_storage) ? PIX_FMT_RGBF : PIX_FMT_3RGBF;
	} else if ((header.bitpix == 32) && (header.channels == 3)){
		pix_format = (header.normal_pixel_storage) ? PIX_FMT_RGB96 : PIX_FMT_3RGB96;
	} else if ((header.bitpix == 16) && (header.channels == 3)){
		pix_format = (header.normal_pixel_storage) ? PIX_FMT_RGB48 : PIX_FMT_3RGB48;
	} else if ((header.bitpix == 8) && (header.channels == 3)){
		pix_format = (header.normal_pixel_storage) ? PIX_FMT_RGB24 : PIX_FMT_3RGB24;
	} else {
		indigo_error("XISF: Unsupported bitpix (BITPIX= %d)", header.bitpix);
		return nullptr;
	}

	if (header.compression[0] == '\0') {
		indigo_debug("XISF: file_size = %d, required_size = %d", xisf_size, header.data_offset + header.data_size);
		if (header.data_offset + header.data_size > xisf_size) {
			indigo_error("XISF: Wrong size (file_size = %d, required_size = %d)", xisf_size, header.data_offset + header.data_size);
			return nullptr;
		}

		if (header.channels == 1) {
			int bayer_pix_fmt = bayer_to_pix_format(header.bayer_pattern, header.bitpix, sconfig.bayer_pattern);
			if (bayer_pix_fmt != 0) pix_format = bayer_pix_fmt;
		}

		img = create_preview(header.width, header.height, pix_format, (char*)xisf_buffer + header.data_offset, sconfig);
	} else {
		char *xisf_data = (char*)malloc(header.uncompressed_data_size);
		int res = xisf_decompress(xisf_buffer, &header, (uint8_t*)xisf_data);
		if (res != XISF_OK) {
			indigo_error("XISF: Decompression failed res = %d", res);
			free(xisf_data);
			return nullptr;
		}

		if (header.channels == 1) {
			int bayer_pix_fmt = bayer_to_pix_format(header.bayer_pattern, header.bitpix, sconfig.bayer_pattern);
			if (bayer_pix_fmt != 0) pix_format = bayer_pix_fmt;
		}

		// transfer ownership of the decompressed buffer to the preview
		std::shared_ptr<char> owner(xisf_data, [](char *p){ free(p); });
		img = create_preview(header.width, header.height, pix_format, owner, (char*)xisf_data, sconfig);
	}

	indigo_debug("XISF_END");
	return img;
}

static int raw_read_keyword_value(const char *ptr8, char *keyword, char *value) {
	int i;
	int length = strlen(ptr8);

	for (i = 0; i < 8 && ptr8[i] != ' '; i++) {
		keyword[i] = ptr8[i];
	}
	keyword[i] = '\0';

	if (ptr8[8] == '=') {
		i++;
		while (i < length && ptr8[i] == ' ') {
			i++;
		}

		if (i < length) {
			*value++ = ptr8[i];
			i++;
			if (ptr8[i-1] == '\'') {
				for (; i < length && ptr8[i] != '\''; i++) {
					*value++ = ptr8[i];
				}
				*value++ = '\'';
			} else if (ptr8[i-1] == '(') {
				for (; i < length && ptr8[i] != ')'; i++) {
					*value++ = ptr8[i];
				}
				*value++ = ')';
			} else {
				for (; i < length && ptr8[i] != ' ' && ptr8[i] != '/'; i++) {
					*value++ = ptr8[i];
				}
			}
		}
	}
	*value = '\0';
	return 0;
}

preview_image* create_raw_preview(unsigned char *raw_image_buffer, unsigned long raw_size, const stretch_config_t sconfig) {
	unsigned int pix_format = 0;

	if (sizeof(indigo_raw_header) > raw_size) {
		indigo_error("RAW: Image buffer is too short: can not fit the header (%dB)", raw_size);
		return nullptr;
	}

	indigo_debug("RAW_START");
	indigo_raw_header *header = (indigo_raw_header*)raw_image_buffer;
	char *raw_data = (char*)raw_image_buffer + sizeof(indigo_raw_header);
	int bitpix = 0;

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
		break;
	case INDIGO_RAW_RGB48:
		pix_format = PIX_FMT_RGB48;
		break;
	default:
		indigo_error("RAW: Unsupported image format (%d)", header->signature);
		return nullptr;
	}

	/* use embedded keywords */
	uint32_t embedded_bayer_pix_fmt = 0;
	int pixel_count = header->width * header->height;
	int data_size = pixel_count * bitpix/8;
	int extension_length = raw_size - data_size - sizeof(indigo_raw_header);
	indigo_debug("RAW: extension_length = %d", extension_length);
	if (extension_length > 9) {
		char *extension = (char*)indigo_safe_malloc(extension_length + 1);
		char *extension_start = extension;
		strncpy(extension, raw_data + data_size, extension_length);
		if (!strncmp(extension_start, "SIMPLE=T;", 9)) {
			extension_start += 9;
			extension_length -= 9;
			char *pos = NULL;
			while(pos = strchr(extension_start, ';')) {
				char keyword[80];
				char value[80];
				*pos = '\0';
				raw_read_keyword_value(extension_start, keyword, value);
				extension_start = pos+1;
				if (!strcmp(keyword, "BAYERPAT")) {
					indigo_debug("RAW: BAYERPAT = %s", value);
					char *value_start = value + 1;              //get rid of the start quote
					value_start[strlen(value_start)-1] = '\0';  //get rid of the end quote
					embedded_bayer_pix_fmt = bayer_to_pix_format(value_start , bitpix, sconfig.bayer_pattern);
				}
			}
		}
		indigo_safe_free(extension);
	}

	if ((sconfig.bayer_pattern == BAYER_PAT_AUTO || sconfig.bayer_pattern == 0) && bitpix != 0 && embedded_bayer_pix_fmt != 0) {
		pix_format = embedded_bayer_pix_fmt;
	} else if (sconfig.bayer_pattern != BAYER_PAT_AUTO && sconfig.bayer_pattern != 0 && bitpix != 0) {
		int bayer_pix_fmt = bayer_to_pix_format(0, bitpix, sconfig.bayer_pattern);
		if (bayer_pix_fmt != 0) pix_format = bayer_pix_fmt;
	}

	preview_image *img = create_preview(header->width, header->height,
	        pix_format, raw_data, sconfig);

	indigo_debug("RAW_END: raw_data = %p", raw_data);
	return img;
}

// Overload: accept ownership of already-allocated input buffer to avoid extra memcpy.
preview_image* create_preview(int width, int height, int pix_format, std::shared_ptr<char> image_owner, char *image_data, const stretch_config_t sconfig) {
	std::lock_guard<std::recursive_mutex> _preview_lock(g_preview_mutex);
	// formats that can be used directly without rearrangement
	if (
		pix_format == PIX_FMT_Y8 || pix_format == PIX_FMT_Y16 || pix_format == PIX_FMT_Y32 || pix_format == PIX_FMT_F32 ||
		pix_format == PIX_FMT_RGB24 || pix_format == PIX_FMT_RGB48 || pix_format == PIX_FMT_RGB96 || pix_format == PIX_FMT_RGBF
	) {
		// create QImage from external buffer so Qt doesn't call QImageData::create
		// Use QImage-internal buffer to avoid external buffer cleanup races
		preview_image* img = new preview_image(width, height, QImage::Format_RGB32);
		img->m_raw_owner = image_owner;
		img->m_raw_data = image_data;
		img->m_pix_format = pix_format;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
		return img;
	}

	// For other formats (planar, bayer etc) fall back to the existing path which will perform conversion/copy.
	return create_preview(width, height, pix_format, image_data, sconfig);
}

preview_image* create_preview(int width, int height, int pix_format, char *image_data, const stretch_config_t sconfig) {
	std::lock_guard<std::recursive_mutex> _preview_lock(g_preview_mutex);
	// Use QImage-internal buffer to avoid external buffer cleanup races
	preview_image* img = new preview_image(width, height, QImage::Format_RGB32);
	if (pix_format == PIX_FMT_Y8) {
		uint8_t* buf = (uint8_t*)image_data;
		uint8_t* pixmap_data = (uint8_t*)malloc(sizeof(uint8_t) * height * width);
		memcpy(pixmap_data, buf, sizeof(uint8_t) * height * width);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_Y8;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_Y16) {
		uint16_t* buf = (uint16_t*)image_data;
		uint16_t* pixmap_data = (uint16_t*)malloc(sizeof(uint16_t) * height * width);
		memcpy(pixmap_data, buf, sizeof(uint16_t) * height * width);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_Y16;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_Y32) {
		uint32_t* buf = (uint32_t*)image_data;
		uint32_t* pixmap_data = (uint32_t*)malloc(sizeof(uint32_t) * height * width);
		memcpy(pixmap_data, buf, sizeof(uint32_t) * height * width);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_Y32;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_F32) {
		float* buf = (float*)image_data;
		float* pixmap_data = (float*)malloc(sizeof(float) * height * width);
		memcpy(pixmap_data, buf, sizeof(float) * height * width);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_F32;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_3RGB24) {
		int channel_offest = width * height;
		uint8_t* buf = (uint8_t*)image_data;
		uint8_t* pixmap_data = (uint8_t*)malloc(sizeof(uint8_t) * height * width * 3);
		int index = 0;
		int index2 = 0;
		for (int y = 0; y < height; ++y) {
			QCoreApplication::processEvents();
			for (int x = 0; x < width; ++x) {
				pixmap_data[index2 + 0] = buf[index];
				pixmap_data[index2 + 1] = buf[index + channel_offest];
				pixmap_data[index2 + 2] = buf[index + 2 * channel_offest];
				index++;
				index2 += 3;
			}
		}
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB24;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_3RGB48) {
		int channel_offest = width * height;
		uint16_t* buf = (uint16_t*)image_data;
		uint16_t* pixmap_data = (uint16_t*)malloc(sizeof(uint16_t) * height * width * 3);
		int index = 0;
		int index2 = 0;
		for (int y = 0; y < height; ++y) {
			QCoreApplication::processEvents();
			for (int x = 0; x < width; ++x) {
				pixmap_data[index2 + 0] = buf[index];
				pixmap_data[index2 + 1] = buf[index + channel_offest];
				pixmap_data[index2 + 2] = buf[index + 2 * channel_offest];
				index++;
				index2 += 3;
			}
		}
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB48;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_3RGB96) {
		int channel_offest = width * height;
		uint32_t* buf = (uint32_t*)image_data;
		uint32_t* pixmap_data = (uint32_t*)malloc(sizeof(uint32_t) * height * width * 3);
		int index = 0;
		int index2 = 0;
		for (int y = 0; y < height; ++y) {
			QCoreApplication::processEvents();
			for (int x = 0; x < width; ++x) {
				pixmap_data[index2 + 0] = buf[index];
				pixmap_data[index2 + 1] = buf[index + channel_offest];
				pixmap_data[index2 + 2] = buf[index + 2 * channel_offest];
				index++;
				index2 += 3;
			}
		}
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB96;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_3RGBF) {
		int channel_offest = width * height;
		float* buf = (float*)image_data;
		float* pixmap_data = (float*)malloc(sizeof(float) * height * width * 3);
		int index = 0;
		int index2 = 0;
		for (int y = 0; y < height; ++y) {
			QCoreApplication::processEvents();
			for (int x = 0; x < width; ++x) {
				pixmap_data[index2 + 0] = buf[index];
				pixmap_data[index2 + 1] = buf[index + channel_offest];
				pixmap_data[index2 + 2] = buf[index + 2 * channel_offest];
				index++;
				index2 += 3;
			}
		}
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGBF;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_RGB24) {
		uint8_t* buf = (uint8_t*)image_data;
		uint8_t* pixmap_data = (uint8_t*)malloc(sizeof(uint8_t) * height * width * 3);
		memcpy(pixmap_data, buf, sizeof(uint8_t) * height * width * 3);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB24;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_RGB48) {
		uint16_t* buf = (uint16_t*)image_data;
		uint16_t* pixmap_data = (uint16_t*)malloc(sizeof(uint16_t) * height * width * 3);
		memcpy(pixmap_data, buf, sizeof(uint16_t) * height * width * 3);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB48;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_RGB96) {
		uint32_t* buf = (uint32_t*)image_data;
		uint32_t* pixmap_data = (uint32_t*)malloc(sizeof(uint32_t) * height * width * 3);
		memcpy(pixmap_data, buf, sizeof(uint32_t) * height * width * 3);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB96;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if (pix_format == PIX_FMT_RGBF) {
		float* buf = (float*)image_data;
		float* pixmap_data = (float*)malloc(sizeof(float) * height * width * 3);
		memcpy(pixmap_data, buf, sizeof(float) * height * width * 3);
		img->m_raw_owner = std::shared_ptr<char>((char*)pixmap_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGBF;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if ((pix_format == PIX_FMT_SBGGR8) || (pix_format == PIX_FMT_SGBRG8) ||
		       (pix_format == PIX_FMT_SGRBG8) || (pix_format == PIX_FMT_SRGGB8)) {
		uint8_t* rgb_data = (uint8_t*)malloc(width*height*3);
		parallel_debayer((uint8_t*)image_data, width, height, get_bayer_offsets(pix_format), rgb_data);

		img->m_raw_owner = std::shared_ptr<char>((char*)rgb_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB24;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if ((pix_format == PIX_FMT_SBGGR16) || (pix_format == PIX_FMT_SGBRG16) ||
		       (pix_format == PIX_FMT_SGRBG16) || (pix_format == PIX_FMT_SRGGB16)) {
		uint16_t* rgb_data = (uint16_t*)malloc(width*height*6);
		parallel_debayer((uint16_t*)image_data, width, height, get_bayer_offsets(pix_format), rgb_data);

		img->m_raw_owner = std::shared_ptr<char>((char*)rgb_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB48;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if ((pix_format == PIX_FMT_SBGGR32) || (pix_format == PIX_FMT_SGBRG32) ||
		       (pix_format == PIX_FMT_SGRBG32) || (pix_format == PIX_FMT_SRGGB32)) {
		uint32_t* rgb_data = (uint32_t*)malloc(width*height*12);
		parallel_debayer((uint32_t*)image_data, width, height, get_bayer_offsets(pix_format), rgb_data);

		img->m_raw_owner = std::shared_ptr<char>((char*)rgb_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGB96;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else if ((pix_format == PIX_FMT_SBGGRF) || (pix_format == PIX_FMT_SGBRGF) ||
		       (pix_format == PIX_FMT_SGRBGF) || (pix_format == PIX_FMT_SRGGBF)) {
		float* rgb_data = (float*)malloc(width*height*12);
		parallel_debayer((float*)image_data, width, height, get_bayer_offsets(pix_format), rgb_data);

		img->m_raw_owner = std::shared_ptr<char>((char*)rgb_data, [](char *p){ free(p); });
		img->m_raw_data = img->m_raw_owner.get();
		img->m_pix_format = PIX_FMT_RGBF;
		img->m_height = height;
		img->m_width = width;

		stretch_preview(img, sconfig);
	} else {
		char *c = (char*)&pix_format;
		indigo_error("%s(): Unsupported pixel format (%c%c%c%c)", __FUNCTION__, c[0], c[1], c[2], c[3]);
		delete img;
		img = nullptr;
	}
	return img;
}

void stretch_preview(preview_image *img, const stretch_config_t sconfig) {
	std::lock_guard<std::recursive_mutex> _preview_lock(g_preview_mutex);
	if (
		img->m_pix_format == PIX_FMT_Y8 ||
		img->m_pix_format == PIX_FMT_Y16 ||
		img->m_pix_format == PIX_FMT_Y32 ||
		img->m_pix_format == PIX_FMT_F32 ||
		img->m_pix_format == PIX_FMT_RGB24 ||
		img->m_pix_format == PIX_FMT_RGB48 ||
		img->m_pix_format == PIX_FMT_RGB96 ||
		img->m_pix_format == PIX_FMT_RGBF
	) {
		Stretcher s(img->m_width, img->m_height, img->m_pix_format);
		StretchParams sp;
		sp.grey_red.highlights = sp.green.highlights = sp.blue.highlights = 0.9;
		if (sconfig.stretch_level > 0) {
			sp = s.computeParams((const uint8_t*)img->m_raw_data, stretch_params_lut[sconfig.stretch_level].brightness, stretch_params_lut[sconfig.stretch_level].contrast);
		}
		indigo_debug("Stretch level: %d, params %f %f %f\n", sconfig.stretch_level, sp.grey_red.shadows, sp.grey_red.midtones, sp.grey_red.highlights);

		if (sconfig.balance) {
			sp.refChannel = &sp.green;
		}
		s.setParams(sp);
		s.stretch((const uint8_t*)img->m_raw_data, img, 1);
	} else {
		char *c = (char*)&img->m_pix_format;
		indigo_error("%s(): Unsupported pixel format (%c%c%c%c)", __FUNCTION__, c[0], c[1], c[2], c[3]);
	}
}

preview_image* create_preview(indigo_property *property, indigo_item *item, const stretch_config_t sconfig) {
	preview_image *preview = nullptr;
	if (property->type == INDIGO_BLOB_VECTOR ) { //&& property->state == INDIGO_OK_STATE) {
		preview = create_preview(item, sconfig);
	}
	return preview;
}

preview_image* create_preview(indigo_item *item, const stretch_config_t sconfig) {
	return create_preview((unsigned char*)item->blob.value, item->blob.size, item->blob.format, sconfig);
}

preview_image* create_preview(unsigned char *data, size_t size, const char* format, const stretch_config_t sconfig) {
	preview_image *preview = nullptr;
	if (data != NULL && format != NULL) {
		if ((((uint8_t *)data)[0] == 0xFF && ((uint8_t *)data)[1] == 0xD8 && ((uint8_t *)data)[2] == 0xFF)) {
			preview = create_jpeg_preview(data, size);
		} else if (!strncmp((const char*)data, "SIMPLE", 6)) {
			preview = create_fits_preview(data, size, sconfig);
		} else if (!strncmp((const char*)data, "RAW", 3)) {
			preview = create_raw_preview(data, size, sconfig);
		} else if (!strncmp((const char*)data, "XISF0100", 8)) {
			preview = create_xisf_preview(data, size, sconfig);
		//} else if (!strncmp((const char*)data, "II*", 3) || !strncmp((const char*)data, "MM*", 3)) {
		//	preview = create_tiff_preview(data, size);
		} else if (format[0] != '\0') {
			preview = create_dslr_raw_preview(data, size, sconfig);
			if (preview == nullptr) {
				preview = create_qtsupported_preview(data, size);
			}
		}
	}
	return preview;
}
