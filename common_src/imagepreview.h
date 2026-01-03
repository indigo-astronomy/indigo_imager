// Copyright (c) 2019 Rumen G.Bogdanovski
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
// NEGLIGENCEIMAGEOTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, IMAGE IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _IMAGEPREVIEW_H
#define _IMAGEPREVIEW_H

#include <QImage>
#include <QHash>
#include <indigo/indigo_client.h>
#include <pixelformat.h>
#include <image_preview_lut.h>
#include <coordconv.h>
#include <stretcher.h>
#include <memory>

#if !defined(INDIGO_WINDOWS)
#define USE_LIBJPEG
#endif
#if defined(USE_LIBJPEG)
#include <jpeglib.h>
#endif

class preview_image: public QImage {
public:
	preview_image():
		m_raw_data(nullptr),
		m_width(0),
		m_height(0),
		m_pix_format(0),
		m_center_ra(0),
		m_center_dec(0),
		m_telescope_ra(0),
		m_telescope_dec(0),
		m_rotation_angle(0),
		m_parity(0),
		m_pix_scale(0)
	{};

	//preview_image(preview_image &&other) = delete;
	//preview_image(const uchar *data, int width, int height, int bytesPerLine, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr) = delete;
	//preview_image(const char *const xpm[]) = delete;
	//preview_image(uchar *data, int width, int height, int bytesPerLine, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr)= delete;
	//preview_image(const uchar *data, int width, int height, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr)= delete;
	//preview_image(uchar *data, int width, int height, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr)= delete;
	//preview_image(const QSize &size, QImage::Format format)= delete;

	preview_image(int width, int height, QImage::Format format):
		QImage(width, height, format),
		m_raw_data(nullptr),
		m_width(0),
		m_height(0),
		m_pix_format(0),
		m_center_ra(0),
		m_center_dec(0),
		m_telescope_ra(0),
		m_telescope_dec(0),
		m_rotation_angle(0),
		m_parity(0),
		m_pix_scale(0)
	{};

	preview_image(uchar *data, int width, int height, int bytesPerLine, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr):
		QImage(data, width, height, bytesPerLine, format, cleanupFunction, cleanupInfo),
		m_raw_data(nullptr),
		m_width(0),
		m_height(0),
		m_pix_format(0),
		m_center_ra(0),
		m_center_dec(0),
		m_telescope_ra(0),
		m_telescope_dec(0),
		m_rotation_angle(0),
		m_parity(0),
		m_pix_scale(0)
	{ };

	preview_image(preview_image &image): QImage(image) {
		m_width = image.m_width;
		m_height = image.m_height;
		m_pix_format = image.m_pix_format;
		m_center_ra = image.m_center_ra;
		m_center_dec = image.m_center_dec;
		m_telescope_ra = image.m_telescope_ra;
		m_telescope_dec = image.m_telescope_dec;
		m_rotation_angle = image.m_rotation_angle;
		m_parity = image.m_parity;
		m_pix_scale = image.m_pix_scale;

		m_raw_owner = image.m_raw_owner; // share the underlying buffer
		m_raw_data = image.m_raw_data;
	};

	preview_image& operator=(preview_image &image) {
		QImage::operator=(image);
		m_width = image.m_width;
		m_height = image.m_height;
		m_pix_format = image.m_pix_format;
		m_center_ra = image.m_center_ra;
		m_center_dec = image.m_center_dec;
		m_telescope_ra = image.m_telescope_ra;
		m_telescope_dec = image.m_telescope_dec;
		m_rotation_angle = image.m_rotation_angle;
		m_parity = image.m_parity;
		m_pix_scale = image.m_pix_scale;

		// share buffer instead of copying
		m_raw_owner = image.m_raw_owner;
		m_raw_data = image.m_raw_data;
		return *this;
	}

	~preview_image() {
		m_raw_owner.reset();
	};

	int pixel_value(int x, int y, double &r, double &g, double &b) const {
		if (m_raw_data == nullptr) {
			if (pixelFormat().colorModel() == 2) {
				r = g = b = -1;
				return PIX_FMT_INDEX;
			}
			QRgb rgb = pixel(x,y);
			r = qRed(rgb);
			g = qGreen(rgb);
			b = qBlue(rgb);
			return PIX_FMT_RGB24;
		}

		if (x < 0 || x >= m_width || y < 0 || y >= m_height) return 0;
		if (m_pix_format == PIX_FMT_Y8) {
			uint8_t* pixels = (uint8_t*) m_raw_data;
			r = pixels[y * m_width + x];
			g = -1;
			b = -1;
		} else if (m_pix_format == PIX_FMT_Y16) {
			uint16_t* pixels = (uint16_t*) m_raw_data;
			r = pixels[y * m_width + x];
			g = -1;
			b = -1;
		} else if (m_pix_format == PIX_FMT_Y32) {
			uint32_t* pixels = (uint32_t*) m_raw_data;
			r = pixels[y * m_width + x];
			g = -1;
			b = -1;
		} else if (m_pix_format == PIX_FMT_F32) {
			float* pixels = (float*) m_raw_data;
			r = pixels[y * m_width + x];
			g = -1;
			b = -1;
		} else if (m_pix_format == PIX_FMT_RGB24) {
			uint8_t* pixels = (uint8_t*) m_raw_data;
			r = pixels[3 * (y * m_width + x)];
			g = pixels[3 * (y * m_width + x) + 1];
			b = pixels[3 * (y * m_width + x) + 2];
		} else if (m_pix_format == PIX_FMT_RGB48) {
			uint16_t* pixels = (uint16_t*) m_raw_data;
			r = pixels[3 * (y * m_width + x)];
			g = pixels[3 * (y * m_width + x) + 1];
			b = pixels[3 * (y * m_width + x) + 2];
		} else if (m_pix_format == PIX_FMT_RGB96) {
			uint32_t* pixels = (uint32_t*) m_raw_data;
			r = pixels[3 * (y * m_width + x)];
			g = pixels[3 * (y * m_width + x) + 1];
			b = pixels[3 * (y * m_width + x) + 2];
		} else if (m_pix_format == PIX_FMT_RGBF) {
			float* pixels = (float*) m_raw_data;
			r = pixels[3 * (y * m_width + x)];
			g = pixels[3 * (y * m_width + x) + 1];
			b = pixels[3 * (y * m_width + x) + 2];
		}
		return m_pix_format;
	};

	void set_wcs_data(double center_ra, double center_dec, double telescope_ra, double telescope_dec, double rotation_angle, int parity, double pix_scale) {
		m_center_ra = center_ra;
		m_center_dec = center_dec;
		m_telescope_ra = telescope_ra;
		m_telescope_dec = telescope_dec;
		m_rotation_angle = rotation_angle;
		m_parity = parity;
		m_pix_scale = pix_scale;
	};

	int wcs_data(double x, double y, double *ra, double *dec, double *telescope_ra = nullptr, double *telescope_dec = nullptr, double *pix_scale = nullptr) const {
		if (m_pix_scale == 0) return -1;
		double center_x = width() / 2.0;
		double center_y = height() / 2.0;
		double dxr = (x - center_x);
		double dyr = (y - center_y);
		double dx, dy;

		if (derotate_xy(dxr, dyr, m_rotation_angle, m_parity, &dx, &dy)) return -1;

		double radius = sqrt(dx * dx + dy * dy);
		double gn_radius = gn_R0(radius, m_pix_scale);

		if (gn_radius > 0) {
			gn_xy2radec(dx, dy, 0, 0, m_center_ra, m_center_dec, gn_radius, ra, dec);
			if (telescope_ra && telescope_dec) {
				gn_xy2radec(dx, dy, 0, 0, m_telescope_ra, m_telescope_dec, gn_radius, telescope_ra, telescope_dec);
			}
		} else {
			*ra = m_center_ra;
			*dec = m_center_dec;
			if (telescope_ra && telescope_dec) {
				*telescope_ra = m_telescope_ra;
				*telescope_ra = m_telescope_dec;
			}
		}

		if (pix_scale) *pix_scale = m_pix_scale;
		if (telescope_ra && telescope_dec) {
			indigo_debug(
				"%s(): x = %f, y = %f => RA = %s, Dec = %s (telescope RA = %s, Dec = %s)", __FUNCTION__,
				x, y,
				indigo_dtos(*ra / 15, "%dh %02d' %04.1f\""), indigo_dtos(*dec, "%d° %02d' %04.1f\""),
				indigo_dtos(*telescope_ra / 15, "%dh %02d' %04.1f\""), indigo_dtos(*telescope_dec, "%d° %02d' %04.1f\"")
			);
		} else {
			indigo_debug(
				"%s(): x = %f, y = %f => RA = %s, Dec = %s", __FUNCTION__,
				x, y,
				indigo_dtos(*ra / 15, "%dh %02d' %04.1f\""), indigo_dtos(*dec, "%d° %02d' %04.1f\"")
			);
		}
		return 0;
	};

	int image_center(double *ra, double *dec) const {
		if (m_pix_scale == 0) return -1;
		if (ra) *ra = m_center_ra;
		if (dec) *dec = m_center_dec;
		return 0;
	};

	char *m_raw_data;
	std::shared_ptr<char> m_raw_owner;
	int m_width;
	int m_height;
	int m_pix_format;
	double m_center_ra;
	double m_center_dec;
	double m_telescope_ra;
	double m_telescope_dec;
	double m_rotation_angle;
	int m_parity;
	double m_pix_scale;
	StretchParams m_strech_params;
};

int get_bayer_offsets(uint32_t pix_format);
template <typename T> void parallel_debayer(T *input_buffer, int width, int height, int offsets, T *output_buffer);

preview_image* create_jpeg_preview(unsigned char *jpg_buffer, unsigned long jpg_size);
preview_image* create_fits_preview(unsigned char *fits_buffer, unsigned long fits_size, const stretch_config_t sconfig);
preview_image* create_xisf_preview(unsigned char *xisf_buffer, unsigned long xisf_size, const stretch_config_t sconfig);
preview_image* create_raw_preview(unsigned char *raw_image_buffer, unsigned long raw_size, const stretch_config_t sconfig);
preview_image* create_preview(unsigned char *data, size_t size, const char* format, const stretch_config_t sconfig);
preview_image* create_preview(int width, int height, int pixel_format, char *image_data, const stretch_config_t sconfig);
preview_image* create_preview(int width, int height, int pixel_format, std::shared_ptr<char> image_owner, char *image_data, const stretch_config_t sconfig);
preview_image* create_preview(indigo_property *property, indigo_item *item, const stretch_config_t sconfig);
preview_image* create_preview(indigo_item *item, const stretch_config_t sconfig);
void stretch_preview(preview_image *img, const stretch_config_t sconfig);

#endif /* _IMAGEPREVIEW_H */
