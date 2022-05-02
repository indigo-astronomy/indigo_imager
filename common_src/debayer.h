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
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _DEBAYER_H
#define _DEBAYER_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void bayer_to_rgb24(const unsigned char *bayer,
  unsigned char *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_bgr24(const unsigned char *bayer,
  unsigned char *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_rgb48(const uint16_t *bayer,
  uint16_t *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_bgr48(const uint16_t *bayer,
  uint16_t *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_rgb96(const uint32_t *bayer,
	uint32_t *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_bgr96(const uint32_t *bayer,
	uint32_t *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_rgbf(const float *bayer,
	float *rgb, int width, int height, unsigned int pixfmt);

void bayer_to_bgrf(const float *bayer,
	float *rgb, int width, int height, unsigned int pixfmt);

#ifdef __cplusplus
}
#endif

#endif /* _DEBAYER_H */
