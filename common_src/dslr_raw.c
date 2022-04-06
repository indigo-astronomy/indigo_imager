#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <indigo/indigo_bus.h>
#include <dslr_raw.h>

static int image_debayered_data(libraw_data_t *raw_data, libraw_image_s *libraw_image) {
	int rc;
	libraw_processed_image_t *processed_image = NULL;

	rc = libraw_raw2image(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_raw2image failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}

	rc = libraw_dcraw_process(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_dcraw_process failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}

	processed_image = libraw_dcraw_make_mem_image(raw_data, &rc);
	if (!processed_image) {
		indigo_error("[rc:%d] libraw_dcraw_make_mem_image failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}

	if (processed_image->type != LIBRAW_IMAGE_BITMAP) {
		indigo_error("input data is not of type LIBRAW_IMAGE_BITMAP");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}

	if (processed_image->colors != 3) {
		indigo_error("debayered data has not 3 colors");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}

	if (processed_image->bits != 16) {
		indigo_error("16 bit is supported only");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}

	libraw_image->width = processed_image->width;
	libraw_image->height = processed_image->height;
	libraw_image->size = processed_image->data_size;
	libraw_image->colors = processed_image->colors;

	if (libraw_image->data)
		free(libraw_image->data);

	libraw_image->data = malloc(libraw_image->size);
	if (!libraw_image->data) {
		indigo_error("%s", strerror(errno));
		rc = errno;
		goto cleanup;
	}

	memcpy(libraw_image->data, processed_image->data, libraw_image->size);

cleanup:
	libraw_dcraw_clear_mem(processed_image);
	return rc;
}

static int image_bayered_data(libraw_data_t *raw_data, libraw_image_s *libraw_image, const bool binning) {
	uint16_t *data;
	uint16_t width, height, raw_width;
	uint32_t npixels;
	uint32_t offset;
	size_t size;
	uint32_t i = 0;

	if (raw_data->sizes.iheight > raw_data->sizes.raw_height) {
		indigo_error("Images with raw_height < image_height are not supported");
		return LIBRAW_UNSPECIFIED_ERROR;
	}

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
	libraw_image->colors = false;
	memset(libraw_image->bayer_pattern, 0, sizeof(libraw_image->bayer_pattern));
	libraw_image->size = 0;
	libraw_image->data = NULL;

#if !defined(INDIGO_WINDOWS)
	clock_t start = clock();
#endif

	raw_data = libraw_init(0);

	/* Linear 16-bit output. */
	raw_data->params.output_bps = 16;
	/* Use simple interpolation (0) to debayer. > 20 will not debyer */
	raw_data->params.user_qual = 0;
	/* Disable four color space. */
	raw_data->params.four_color_rgb = 0;
	/* Disable LibRaw's default histogram transformation. */
	raw_data->params.no_auto_bright = 0;
	/* Disable LibRaw's default gamma curve transformation, */
	raw_data->params.gamm[0] = raw_data->params.gamm[1] = 1.0;
	/* Do not apply an embedded color profile, enabled by LibRaw by default. */
	raw_data->params.use_camera_matrix = 0;
	/* Disable automatic white balance obtained after averaging over the entire image. */
	raw_data->params.use_auto_wb = 0;
	/* Disable white balance from the camera (if possible). */
	raw_data->params.use_camera_wb = 0;

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

	libraw_image->bayer_pattern[0] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 2, 2)];
	libraw_image->bayer_pattern[1] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 2, 3)];
	libraw_image->bayer_pattern[2] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 3, 2)];
	libraw_image->bayer_pattern[3] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 3, 3)];

	indigo_error("Maker       : %s, Model      : %s", raw_data->idata.make, raw_data->idata.model);
	indigo_error("Norm Maker  : %s, Norm Model : %s", raw_data->idata.normalized_make, raw_data->idata.normalized_model);
	indigo_error("width       = %d, height     = %d", raw_data->sizes.width, raw_data->sizes.height);
	indigo_error("iwidth      = %d, iheight    = %d", raw_data->sizes.iwidth, raw_data->sizes.iheight);
	indigo_error("raw_width   = %d, raw_height = %d", raw_data->sizes.raw_width, raw_data->sizes.raw_height);
	indigo_error("left_margin = %d, top_margin = %d", raw_data->sizes.left_margin, raw_data->sizes.top_margin);
	indigo_error("bayerpat    : %s, cdesc      : %s", libraw_image->bayer_pattern, raw_data->idata.cdesc);

	if (raw_data->params.user_qual > 20) {
		rc = image_bayered_data(raw_data, libraw_image, false);
		if (rc) goto cleanup;
		libraw_image->debayered = false;
	} else {
		rc = image_debayered_data(raw_data, libraw_image);
		if (rc) goto cleanup;
		libraw_image->debayered = true;
	}

#if !defined(INDIGO_WINDOWS)
	indigo_error(
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
#endif

cleanup:
	libraw_free_image(raw_data);
	libraw_recycle(raw_data);
	libraw_close(raw_data);

	return rc;
}
