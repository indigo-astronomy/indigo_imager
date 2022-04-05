#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <indigo/indigo_bus.h>
#include <dslr_raw.h>

static int progress_cb(void *callback_data, enum LibRaw_progress stage, int iteration, int expected) {
	(void)callback_data;
	indigo_debug("libraw: %s, step %i/%i",libraw_strprogress(stage), iteration + 1, expected);
	return 0;
}

static int image_bayered_data(libraw_data_t *raw_data, libraw_image_s *libraw_image, const bool binning) {
	uint16_t *data;
	uint16_t width, height, raw_width;
	uint32_t npixels;
	uint32_t offset;
	size_t size;
	uint32_t i = 0;

	if (binning) {
		width = raw_data->sizes.iwidth % 2 == 0 ? raw_data->sizes.iwidth : raw_data->sizes.iwidth - 1;
		height = raw_data->sizes.iheight % 2 == 0 ? raw_data->sizes.iheight : raw_data->sizes.iheight - 1;
	} else {
		width = raw_data->sizes.iwidth;
		height = raw_data->sizes.iheight;
	}
	raw_width = raw_data->sizes.raw_width;
	npixels = width * height;
	offset = raw_width * raw_data->rawdata.sizes.top_margin +
		raw_data->rawdata.sizes.left_margin;
	size = binning ? npixels / 4 * sizeof(uint16_t) : npixels * sizeof(uint16_t);

	data = (uint16_t *)calloc(1, size);
	if (!data) {
		indigo_error("%s", strerror(errno));
		return -errno;
	}
	libraw_image->width = binning ? width / 2 : width;
	libraw_image->height = binning ? height / 2 : height;
	libraw_image->size = size;

	int c = binning ? 2 : 1;
#ifdef FIT_FORMAT_AMATEUR_CCD
	for (int row = 0; row < height; row += c) {
#else
	for (int row = height - 1; row >= 0; row -= c) {
#endif
		for (int col = 0; col < width; col += c) {
			if (binning) {
				data[i++] = (
					raw_data->rawdata.raw_image[offset + col + 0 + (raw_width * row)] +     /* R */
					raw_data->rawdata.raw_image[offset + col + 1 + (raw_width * row)] +     /* G */
					raw_data->rawdata.raw_image[offset + col + 2 + (raw_width * row)] +     /* G */
					raw_data->rawdata.raw_image[offset + col + 3 + (raw_width * row)]       /* B */
				) / 4;
			} else {
				data[i++] = raw_data->rawdata.raw_image[offset + col + (raw_width * row)];
			}
		}
	}

	libraw_image->data = data;
	libraw_image->colors = 1;

	return 0;
}

int process_dslr_raw_image(void *buffer, size_t buffer_size, libraw_image_s *libraw_image) {
	int rc;
	libraw_data_t *raw_data;

	libraw_image->width = 0;
	libraw_image->height = 0;
	libraw_image->bits = 16;
	libraw_image->colors = 0;
	memset(libraw_image->bayer_pattern, 0, sizeof(libraw_image->bayer_pattern));
	libraw_image->size = 0;
	libraw_image->data = NULL;

	//clock_t start = clock();
	raw_data = libraw_init(0);

	/* Linear 16-bit output. */
	raw_data->params.output_bps = 16;
	/* Do not debayer. */
	raw_data->params.user_qual = 254;
	/* Disable four color space. */
	raw_data->params.four_color_rgb = 0;
	/* Disable LibRaw's default histogram transformation. */
	raw_data->params.no_auto_bright = 1;
	/* Disable LibRaw's default gamma curve transformation, */
	raw_data->params.gamm[0] = raw_data->params.gamm[1] = 1.0;
	/* Do not apply an embedded color profile, enabled by LibRaw by default. */
	raw_data->params.use_camera_matrix = 0;
	/* Disable automatic white balance obtained after averaging over the entire image. */
	raw_data->params.use_auto_wb = 0;
	/* Disable white balance from the camera (if possible). */
	raw_data->params.use_camera_wb = 0;

	libraw_set_progress_handler(raw_data, &progress_cb, NULL);

	rc = libraw_open_buffer(raw_data, buffer, buffer_size);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_open_buffer failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}

	rc = libraw_unpack(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error( "[rc:%d] libraw_unpack failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}

	libraw_image->bayer_pattern[0] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 0, 0)];
	libraw_image->bayer_pattern[1] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 0, 1)];
	libraw_image->bayer_pattern[2] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 1, 0)];
	libraw_image->bayer_pattern[3] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 1, 1)];

	rc = image_bayered_data(raw_data, libraw_image, false);
	if (rc) goto cleanup;

	/*
	indigo_debug(
		"libraw conversion in %gs, input size: "
		"%d bytes, unpacked + (de)bayered output size: "
		"%d bytes, bayer pattern '%s', "
		"dimension: %d x %d, "
		"bits: %d, colors: %d",
		(clock() - start) / (double)CLOCKS_PER_SEC,
		buffer_size,
		libraw_image->size,
		libraw_image->bayer_pattern,
		libraw_image->width, libraw_image->height,
		libraw_image->bits, libraw_image->colors
	);
	*/
cleanup:
	libraw_free_image(raw_data);
	libraw_recycle(raw_data);
	libraw_close(raw_data);

	return rc;
}
