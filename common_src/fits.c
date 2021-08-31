// Copyright (c) 2019 Rumen G.Bogdanovski
// All rights reserved.
//
// Parts based on Paras Chadha code
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

#include "fits.h"
#include <string.h>
#include <stdio.h>
#include <indigo/indigo_bus.h>

static int fits_header_init(fits_header *header, fits_header_state state) {
	header->state = state;
	header->naxis_index = 0;
	header->blank_found = 0;
	header->pcount = 0;
	header->gcount = 1;
	header->groups = 0;
	header->rgb = 0;
	header->bayerpat[0] = '\0';
	header->xbayeroff = 3;
	header->ybayeroff = 4;
	header->image_extension = 0;
	header->bscale = 1.0;
	header->bzero = 0;
	header->data_min = 0;
	header->data_min_found = 0;
	header->data_max = 0;
	header->data_max_found = 0;
	header->data_offset = 0;
	return 0;
}


static int read_keyword_value(const uint8_t *ptr8, char *keyword, char *value) {
	int i;

	for (i = 0; i < 8 && ptr8[i] != ' '; i++) {
		keyword[i] = ptr8[i];
	}
	keyword[i] = '\0';

	if (ptr8[8] == '=') {
		i = 10;
		while (i < 80 && ptr8[i] == ' ') {
			i++;
		}

		if (i < 80) {
			*value++ = ptr8[i];
			i++;
			if (ptr8[i-1] == '\'') {
				for (; i < 80 && ptr8[i] != '\''; i++) {
					*value++ = ptr8[i];
				}
				*value++ = '\'';
			} else if (ptr8[i-1] == '(') {
				for (; i < 80 && ptr8[i] != ')'; i++) {
					*value++ = ptr8[i];
				}
				*value++ = ')';
			} else {
				for (; i < 80 && ptr8[i] != ' ' && ptr8[i] != '/'; i++) {
					*value++ = ptr8[i];
				}
			}
		}
	}
	*value = '\0';
	return 0;
}


#define CHECK_KEYWORD(key) \
	if (strcmp(keyword, key)) { \
		indigo_error("Expected %s keyword, found %s = %s\n", key, keyword, value); \
		return FITS_INVALIDDATA; \
	}

#define CHECK_VALUE(key, val) \
	if (sscanf(value, "%d", &header->val) != 1) { \
		indigo_error("Invalid value of %s keyword, %s = %s\n", key, keyword, value); \
		return FITS_INVALIDDATA; \
	}


static int fits_header_parse_line(fits_header *header, const uint8_t line[80]) {
	int dim_no, ret;
	int64_t t;
	double d;
	char keyword[10], value[72], c;

	read_keyword_value(line, keyword, value);
	switch (header->state) {
	case STATE_SIMPLE:
		CHECK_KEYWORD("SIMPLE");

		if (value[0] == 'F') {
			indigo_error("not a standard FITS file\n");
		} else if (value[0] != 'T') {
			indigo_error("invalid value of SIMPLE keyword, SIMPLE = %c\n", value[0]);
			return FITS_INVALIDDATA;
		}

		header->state = STATE_BITPIX;
		break;
	case STATE_XTENSION:
		CHECK_KEYWORD("XTENSION");

		if (!strcmp(value, "'IMAGE   '")) {
			header->image_extension = 1;
		}

		header->state = STATE_BITPIX;
		break;
	case STATE_BITPIX:
		CHECK_KEYWORD("BITPIX");
		CHECK_VALUE("BITPIX", bitpix);

		switch(header->bitpix) {
			case   8:
			case  16:
			case  32:
			case -32:
			case  64:
			case -64: break;
			default:
				indigo_error("invalid value of BITPIX %d\n", header->bitpix); \
				return FITS_INVALIDDATA;
		}

		header->state = STATE_NAXIS;
		break;
	case STATE_NAXIS:
		CHECK_KEYWORD("NAXIS");
		CHECK_VALUE("NAXIS", naxis);

		if (header->naxis) {
			header->state = STATE_NAXIS_N;
		} else {
			header->state = STATE_REST;
		}
		break;
	case STATE_NAXIS_N:
		ret = sscanf(keyword, "NAXIS%d", &dim_no);
		if (ret != 1 || dim_no != header->naxis_index + 1) {
			indigo_error("expected NAXIS%d keyword, found %s = %s\n", header->naxis_index + 1, keyword, value);
			return FITS_INVALIDDATA;
		}

		if (sscanf(value, "%d", &header->naxisn[header->naxis_index]) != 1) {
			indigo_error("invalid value of NAXIS%d keyword, %s = %s\n", header->naxis_index + 1, keyword, value);
			return FITS_INVALIDDATA;
		}

		header->naxis_index++;
		if (header->naxis_index == header->naxis) {
			header->state = STATE_REST;
		}
		break;
	case STATE_REST:
		if (!strcmp(keyword, "BLANK") && sscanf(value, "%"SCNd64"", &t) == 1) {
			header->blank = t;
			header->blank_found = 1;
		} else if (!strcmp(keyword, "BSCALE") && sscanf(value, "%lf", &d) == 1) {
			header->bscale = d;
		} else if (!strcmp(keyword, "BZERO") && sscanf(value, "%lf", &d) == 1) {
			header->bzero = d;
		} else if (!strcmp(keyword, "CTYPE3") && !strncmp(value, "'RGB", 4)) {
			header->rgb = 1;
		} else if (!strcmp(keyword, "BAYERPAT")) {
			strncpy(header->bayerpat, value+1, 4);
			header->bayerpat[4] = '\0';
		} else if (!strcmp(keyword, "XBAYROFF") && sscanf(value, "%lf", &d) == 1) {
			header->xbayeroff = d;
		} else if (!strcmp(keyword, "YBAYROFF") && sscanf(value, "%lf", &d) == 1) {
			header->ybayeroff = d;
		} else if (!strcmp(keyword, "DATAMIN") && sscanf(value, "%lf", &d) == 1) {
			header->data_min_found = 1;
			header->data_min = d;
		} else if (!strcmp(keyword, "END")) {
			return 1;
		} else if (!strcmp(keyword, "GROUPS") && sscanf(value, "%c", &c) == 1) {
			header->groups = (c == 'T');
		} else if (!strcmp(keyword, "GCOUNT") && sscanf(value, "%"SCNd64"", &t) == 1) {
			header->gcount = t;
		} else if (!strcmp(keyword, "PCOUNT") && sscanf(value, "%"SCNd64"", &t) == 1) {
			header->pcount = t;
		}
		break;
	}
	return FITS_OK;
}


int fits_read_header(const uint8_t *fits_data, int fits_size, fits_header *header) {
	const uint8_t *ptr8 = fits_data;
	int lines_read, i, ret;
	size_t size;

	if (fits_size < 2880) return FITS_INVALIDDATA;

	lines_read = 1; // to account for first header line, SIMPLE or XTENSION which is not included in packet...
	fits_header_init(header, STATE_SIMPLE);
	do {
		ret = fits_header_parse_line(header, ptr8);
		ptr8 += 80;
		lines_read++;
	} while (!ret);

	if (ret < 0) return ret;

	header->data_offset = (ptr8 + 80) - fits_data + ((((lines_read + 35) / 36) * 36 - lines_read) * 80);

	if (header->rgb && (header->naxis != 3 || (header->naxisn[2] != 3 && header->naxisn[2] != 4))) {
		indigo_error("File contains RGB image but NAXIS = %d and NAXIS3 = %d\n", header->naxis, header->naxisn[2]);
		return FITS_INVALIDDATA;
	}

	if (header->blank_found && (header->bitpix == -32 || header->bitpix == -64)) {
		indigo_log("BLANK keyword found but BITPIX = %d\n. Ignoring BLANK", header->bitpix);
		header->blank_found = 0;
	}

	size = abs(header->bitpix) >> 3;
	for (i = 0; i < header->naxis; i++) {
		if (size && header->naxisn[i] > SIZE_MAX / size) {
			indigo_error("unsupported size of FITS image");
			return FITS_INVALIDDATA;
		}
		size *= header->naxisn[i];
	}
	return FITS_OK;
}

int fits_get_buffer_size(fits_header *header) {
	int size = abs(header->bitpix) / 8;
	for (int i = 0; i < header->naxis; i++){
		size *= header->naxisn[i];
	}
	return size;
}

int fits_process_data(const uint8_t *fits_data, int fits_size, fits_header *header, char *native_data) {
	int little_endian = 1;
	int size = 1;
	for (int i = 0; i < header->naxis; i++){
		size *= header->naxisn[i];
	}

	indigo_debug("size = %d min_size = %d fits_size = %d\n", size, size * header->bitpix/8 + header->data_offset, fits_size);
	if ((size * header->bitpix/8 + header->data_offset) > fits_size) {
		return FITS_INVALIDDATA;
	}

	if (header->bitpix == 16 && header->naxis > 0) {
		short *raw = (short *)(fits_data + header->data_offset);
		short *native = (short *)native_data;
		if (little_endian) {
			for (int i = 0; i < size; i++) {
				*native = (*raw & 0xff) << 8 | (*raw & 0xff00) >> 8;
				*native = (*native + header->bzero) * header->bscale;
				native++;
				raw++;
			}
		} else {
			for (int i = 0; i < size; i++) {
				*native++ = (*raw++ + header->bzero) * header->bscale;
			}
		}
		return FITS_OK;
	} else if (header->bitpix == 8 && header->naxis > 0) {
		uint8_t *raw = (uint8_t *)(fits_data + header->data_offset);
		uint8_t *native = (uint8_t *)native_data;
		for (int i = 0; i < size; i++) {
			*native++ = (*raw++ + header->bzero) * header->bscale;
		}
		return FITS_OK;
	}
	return FITS_INVALIDDATA;
}


int fits_process_data_with_hist(const uint8_t *fits_data, int fits_size, fits_header *header, char *native_data, int *hist) {
	int little_endian = 1;
	int size = 1;

	if (!hist) return FITS_INVALIDPARAM;

	for (int i = 0; i < header->naxis; i++){
		size *= header->naxisn[i];
	}

	indigo_debug("size = %d min_size = %d fits_size = %d\n", size, size * header->bitpix/8 + header->data_offset, fits_size);
	if ((size * header->bitpix/8 + header->data_offset) > fits_size) {
		return FITS_INVALIDDATA;
	}

	if (header->bitpix == 16 && header->naxis > 0) {
		memset(hist, 0, 65536 * sizeof(hist[0]));
		short *raw = (short *)(fits_data + header->data_offset);
		uint16_t *native = (uint16_t *)native_data;
		if (little_endian) {
			for (int i = 0; i < size; i++) {
				*native = (*raw & 0xff) << 8 | (*raw & 0xff00) >> 8;
				*native = (*native + header->bzero) * header->bscale;
				hist[*native]++;
				native++;
				raw++;
			}
		} else {
			for (int i = 0; i < size; i++) {
				*native = (*raw++ + header->bzero) * header->bscale;
				hist[*native]++;
				native++;
			}
		}
		return FITS_OK;
	} else if (header->bitpix == 8 && header->naxis > 0) {
		memset(hist, 0, 256*sizeof(hist[0]));
		uint8_t *raw = (uint8_t *)(fits_data + header->data_offset);
		uint8_t *native = (uint8_t *)native_data;
		for (int i = 0; i < size; i++) {
			*native = (*raw++ + header->bzero) * header->bscale;
			hist[*native]++;
			native++;
		}
		return FITS_OK;
	}
	return FITS_INVALIDDATA;
}
