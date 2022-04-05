// Copyright (c) 2022 Rumen G. Bogdanovski
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

// version history
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

#ifndef _DSLR_RAW_H
#define _DSLR_RAW_H

#include <libraw/libraw.h>

typedef struct {
	uint16_t width;
	uint16_t height;
	size_t size;
	void *data;
	uint8_t bits;
	uint8_t colors;
	char bayer_pattern[5];
} libraw_image_s;

#ifdef __cplusplus
extern "C" {
#endif

#define FIT_FORMAT_AMATEUR_CCD

int process_dslr_raw_image(void *buffer, size_t buffer_size, libraw_image_s *libraw_image);

#ifdef __cplusplus
}
#endif

#endif /* _RAW_TO_FITS_H */
