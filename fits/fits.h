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

#ifndef _FITS_H
#define _FITS_H

#include <inttypes.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum fits_error {
	FITS_OK = 0,
	FITS_INVALIDDATA = -1,
} fits_error;


typedef enum fits_headerState {
	STATE_SIMPLE,
	STATE_XTENSION,
	STATE_BITPIX,
	STATE_NAXIS,
	STATE_NAXIS_N,
	STATE_PCOUNT,
	STATE_GCOUNT,
	STATE_REST,
} fits_header_state;

/**
 * Structure to store the header keywords in FITS file
 */
typedef struct fits_header {
	fits_header_state state;
	unsigned naxis_index;
	int bitpix;
	int64_t blank;
	int blank_found;
	int naxis;
	int naxisn[999];
	int pcount;
	int gcount;
	int groups;
	int rgb; /**< 1 if file contains RGB image, 0 otherwise */
	char bayerpat[5];
	int xbayeroff;
	int ybayeroff;
	int image_extension;
	double bscale;
	double bzero;
	int data_min_found;
	double data_min;
	int data_max_found;
	double data_max;
	int data_offset;
} fits_header;

int fits_read_header(const uint8_t *fits_data, int fits_size, fits_header *header);
int fits_get_buffer_size(fits_header *header);
int fits_process_data(const uint8_t *fits_data, int fits_size, fits_header *header, char *native_data);
int fits_process_data_with_hist(const uint8_t *fits_data, int fits_size, fits_header *header, char *native_data, int *hist);

#ifdef __cplusplus
}
#endif

#endif /* _FITS_H */
