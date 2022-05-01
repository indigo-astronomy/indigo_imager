// Copyright (c) 2022 Rumen G.Bogdanovski
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

#ifndef _XISF_H
#define _XISF_H

#include <inttypes.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum xisf_error {
	XISF_OK = 0,
	XISF_INVALIDDATA = -1,
	XISF_INVALIDPARAM = -2,
	XISF_UNSUPPORTED = -3,
	XISF_NOT_XISF = -4
} xisf_error;

/**
 * Structure to store xisf metadata from the XML header
 */
typedef struct xisf_metadata {
	int bitpix;
	int width;
	int height;
	int channels;
	bool big_endian;            // true for big endian
	bool normal_pixel_storage;  // true for normal "rgbrgb..." pixek storage false for 3 plane rgb
	int data_offset;            // offset where data starts
	int data_size;
	int uncompressed_data_size;
	int shuffle_size;
	float exposure_time;
	float sensor_temperature;
	char observation_time[50];
	char compression[50];       // compression mthod
	char color_space[50];
	char bayer_pattern[10];
	char camera_name[256];
	char image_type[256];
} xisf_metadata;

/**
 * xisf binary header
 */
typedef struct xsif_header {
	char  signature[8];    // 'XISF0100'
	uint32_t xml_length;          // length in bytes of the XML file header
	uint32_t reserved;        // reserved - must be zero
} xisf_header;

int xisf_read_metadata(uint8_t *xisf_data, int xisf_size, xisf_metadata *metadata);
int xisf_decompress(uint8_t *xisf_data, xisf_metadata *metadata, uint8_t *decompressed_data);

#ifdef __cplusplus
}
#endif

#endif /* _XISF_H */
