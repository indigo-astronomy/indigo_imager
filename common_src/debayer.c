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
//
// based on:
// lib4lconvert, video4linux2 format conversion lib
//						 (C) 2008 Hans de Goede <hdegoede@redhat.com>


#include <inttypes.h>
#include <string.h>
#include "debayer.h"
#include "pixelformat.h"

/* insprired by OpenCV's Bayer decoding */
static void border_bayer_line_to_bgr24(
	const unsigned char* bayer, const unsigned char* adjacent_bayer,
	unsigned char *bgr, int width, int start_with_green, int blue_line)
{
	int t0, t1;

	if (start_with_green) {
		/* First pixel */
		if (blue_line) {
			*bgr++ = bayer[1];
			*bgr++ = bayer[0];
			*bgr++ = adjacent_bayer[0];
		} else {
			*bgr++ = adjacent_bayer[0];
			*bgr++ = bayer[0];
			*bgr++ = bayer[1];
		}
		/* Second pixel */
		t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
		t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[1];
			*bgr++ = t0;
			*bgr++ = t1;
		} else {
			*bgr++ = t1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
		}
		bayer++;
		adjacent_bayer++;
		width -= 2;
	} else {
		/* First pixel */
		t0 = (bayer[1] + adjacent_bayer[0] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[0];
			*bgr++ = t0;
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = t0;
			*bgr++ = bayer[0];
		}
		width--;
	}

	if (blue_line) {
		for ( ; width > 2; width -= 2) {
			t0 = (bayer[0] + bayer[2] + 1) >> 1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
			bayer++;
			adjacent_bayer++;

			t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
			t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
			*bgr++ = bayer[1];
			*bgr++ = t0;
			*bgr++ = t1;
			bayer++;
			adjacent_bayer++;
		}
	} else {
		for ( ; width > 2; width -= 2) {
			t0 = (bayer[0] + bayer[2] + 1) >> 1;
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = t0;
			bayer++;
			adjacent_bayer++;

			t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
			t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
			*bgr++ = t1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
			bayer++;
			adjacent_bayer++;
		}
	}

	if (width == 2) {
		/* Second to last pixel */
		t0 = (bayer[0] + bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = t0;
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = t0;
		}
		/* Last pixel */
		t0 = (bayer[1] + adjacent_bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[2];
			*bgr++ = t0;
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = t0;
			*bgr++ = bayer[2];
		}
	} else {
		/* Last pixel */
		if (blue_line) {
			*bgr++ = bayer[0];
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = bayer[0];
		}
	}
}

/* From libdc1394, which on turn was based on OpenCV's Bayer decoding */
static void bayer_to_rgbbgr24(const unsigned char *bayer,
	unsigned char *bgr, int width, int height, unsigned int pixfmt,
	int start_with_green, int blue_line)
{
	(void)pixfmt;
	/* render the first line */
	border_bayer_line_to_bgr24(bayer, bayer + width, bgr, width, start_with_green, blue_line);
	bgr += width * 3;

	/* reduce height by 2 because of the special case top/bottom line */
	for (height -= 2; height; height--) {
		int t0, t1;
		/* (width - 2) because of the border */
		const unsigned char *bayerEnd = bayer + (width - 2);

		if (start_with_green) {
			/* OpenCV has a bug in the next line, which was
				 t0 = (bayer[0] + bayer[width * 2] + 1) >> 1; */
			t0 = (bayer[1] + bayer[width * 2 + 1] + 1) >> 1;
			/* Write first pixel */
			t1 = (bayer[0] + bayer[width * 2] + bayer[width + 1] + 1) / 3;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width];
			} else {
				*bgr++ = bayer[width];
				*bgr++ = t1;
				*bgr++ = t0;
			}

				/* Write second pixel */
			t1 = (bayer[width] + bayer[width + 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
			} else {
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
				*bgr++ = t0;
			}
			bayer++;
		} else {
			/* Write first pixel */
			t0 = (bayer[0] + bayer[width * 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width];
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = bayer[width];
				*bgr++ = t0;
			}
		}

		if (blue_line) {
			for (; bayer <= bayerEnd - 2; bayer += 2) {
				t0 = (bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
				t1 = (bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];

				t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
				t1 = (bayer[width + 1] + bayer[width + 3] +
							1) >> 1;
				*bgr++ = t0;
				*bgr++ = bayer[width + 2];
				*bgr++ = t1;
			}
		} else {
			for (; bayer <= bayerEnd - 2; bayer += 2) {
				t0 = (bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
				t1 = (bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;

				t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
				t1 = (bayer[width + 1] + bayer[width + 3] + 1) >> 1;
				*bgr++ = t1;
				*bgr++ = bayer[width + 2];
				*bgr++ = t0;
			}
		}

		if (bayer < bayerEnd) {
			/* write second to last pixel */
			t0 = (bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
			t1 = (bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;
			}
			/* write last pixel */
			t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width + 2];
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = bayer[width + 2];
				*bgr++ = t0;
			}
			bayer++;
		} else {
			/* write last pixel */
			t0 = (bayer[0] + bayer[width * 2] + 1) >> 1;
			t1 = (bayer[1] + bayer[width * 2 + 1] + bayer[width] + 1) / 3;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;
			}
		}

		/* skip 2 border pixels */
		bayer += 2;

		blue_line = !blue_line;
		start_with_green = !start_with_green;
	}
	/* render the last line */
	border_bayer_line_to_bgr24(bayer + width, bayer, bgr, width, !start_with_green, !blue_line);
}

void bayer_to_rgb24(const unsigned char *bayer,
	unsigned char *bgr, int width, int height, unsigned int pixfmt)
{
	bayer_to_rgbbgr24(bayer, bgr, width, height, pixfmt,
		pixfmt == PIX_FMT_SGBRG8		/* start with green */
			|| pixfmt == PIX_FMT_SGRBG8,
		pixfmt != PIX_FMT_SBGGR8		/* blue line */
			&& pixfmt != PIX_FMT_SGBRG8);
}

void bayer_to_bgr24(const unsigned char *bayer,
	unsigned char *bgr, int width, int height, unsigned int pixfmt)
{
	bayer_to_rgbbgr24(bayer, bgr, width, height, pixfmt,
		pixfmt == PIX_FMT_SGBRG8		/* start with green */
			|| pixfmt == PIX_FMT_SGRBG8,
		pixfmt == PIX_FMT_SBGGR8		/* blue line */
			|| pixfmt == PIX_FMT_SGBRG8);
}


static void border_bayer_line_to_bgr48(
	const uint16_t* bayer, const uint16_t* adjacent_bayer,
	uint16_t *bgr, int width, int start_with_green, int blue_line)
{
	int t0, t1;

	if (start_with_green) {
		/* First pixel */
		if (blue_line) {
			*bgr++ = bayer[1];
			*bgr++ = bayer[0];
			*bgr++ = adjacent_bayer[0];
		} else {
			*bgr++ = adjacent_bayer[0];
			*bgr++ = bayer[0];
			*bgr++ = bayer[1];
		}
		/* Second pixel */
		t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
		t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[1];
			*bgr++ = t0;
			*bgr++ = t1;
		} else {
			*bgr++ = t1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
		}
		bayer++;
		adjacent_bayer++;
		width -= 2;
	} else {
		/* First pixel */
		t0 = (bayer[1] + adjacent_bayer[0] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[0];
			*bgr++ = t0;
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = t0;
			*bgr++ = bayer[0];
		}
		width--;
	}

	if (blue_line) {
		for ( ; width > 2; width -= 2) {
			t0 = (bayer[0] + bayer[2] + 1) >> 1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
			bayer++;
			adjacent_bayer++;

			t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
			t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
			*bgr++ = bayer[1];
			*bgr++ = t0;
			*bgr++ = t1;
			bayer++;
			adjacent_bayer++;
		}
	} else {
		for ( ; width > 2; width -= 2) {
			t0 = (bayer[0] + bayer[2] + 1) >> 1;
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = t0;
			bayer++;
			adjacent_bayer++;

			t0 = (bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
			t1 = (adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
			*bgr++ = t1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
			bayer++;
			adjacent_bayer++;
		}
	}

	if (width == 2) {
		/* Second to last pixel */
		t0 = (bayer[0] + bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = t0;
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = t0;
		}
		/* Last pixel */
		t0 = (bayer[1] + adjacent_bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[2];
			*bgr++ = t0;
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = t0;
			*bgr++ = bayer[2];
		}
	} else {
		/* Last pixel */
		if (blue_line) {
			*bgr++ = bayer[0];
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = bayer[0];
		}
	}
}

/* From libdc1394, which on turn was based on OpenCV's Bayer decoding */
static void bayer_to_rgbbgr48(const uint16_t *bayer,
	uint16_t *bgr, int width, int height, unsigned int pixfmt,
	int start_with_green, int blue_line)
{
	(void)pixfmt;
	/* render the first line */
	border_bayer_line_to_bgr48(bayer, bayer + width, bgr, width, start_with_green, blue_line);
	bgr += width * 3;

	/* reduce height by 2 because of the special case top/bottom line */
	for (height -= 2; height; height--) {
		int t0, t1;
		/* (width - 2) because of the border */
		const uint16_t *bayerEnd = bayer + (width - 2);

		if (start_with_green) {
			/* OpenCV has a bug in the next line, which was
			   t0 = (bayer[0] + bayer[width * 2] + 1) >> 1; */
			t0 = (bayer[1] + bayer[width * 2 + 1] + 1) >> 1;
			/* Write first pixel */
			t1 = (bayer[0] + bayer[width * 2] + bayer[width + 1] + 1) / 3;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width];
			} else {
				*bgr++ = bayer[width];
				*bgr++ = t1;
				*bgr++ = t0;
			}

			/* Write second pixel */
			t1 = (bayer[width] + bayer[width + 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
			} else {
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
				*bgr++ = t0;
			}
			bayer++;
		} else {
			/* Write first pixel */
			t0 = (bayer[0] + bayer[width * 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width];
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = bayer[width];
				*bgr++ = t0;
			}
		}

		if (blue_line) {
			for (; bayer <= bayerEnd - 2; bayer += 2) {
				t0 = (bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
				t1 = (bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];

				t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
				t1 = (bayer[width + 1] + bayer[width + 3] + 1) >> 1;
				*bgr++ = t0;
				*bgr++ = bayer[width + 2];
				*bgr++ = t1;
			}
		} else {
			for (; bayer <= bayerEnd - 2; bayer += 2) {
				t0 = (bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
				t1 = (bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;

				t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
				t1 = (bayer[width + 1] + bayer[width + 3] + 1) >> 1;
				*bgr++ = t1;
				*bgr++ = bayer[width + 2];
				*bgr++ = t0;
			}
		}

		if (bayer < bayerEnd) {
			/* write second to last pixel */
			t0 = (bayer[0] + bayer[2] + bayer[width * 2] +
			bayer[width * 2 + 2] + 2) >> 2;
			t1 = (bayer[1] + bayer[width] +
			bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;
			}
			/* write last pixel */
			t0 = (bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width + 2];
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = bayer[width + 2];
				*bgr++ = t0;
			}
			bayer++;
		} else {
			/* write last pixel */
			t0 = (bayer[0] + bayer[width * 2] + 1) >> 1;
			t1 = (bayer[1] + bayer[width * 2 + 1] + bayer[width] + 1) / 3;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;
			}
		}

		/* skip 2 border pixels */
		bayer += 2;

		blue_line = !blue_line;
		start_with_green = !start_with_green;
	}

	/* render the last line */
	border_bayer_line_to_bgr48(bayer + width, bayer, bgr, width, !start_with_green, !blue_line);
}

void bayer_to_rgb48(const uint16_t *bayer,
	uint16_t *bgr, int width, int height, unsigned int pixfmt)
{
	bayer_to_rgbbgr48(bayer, bgr, width, height, pixfmt,
		pixfmt == PIX_FMT_SGBRG16		/* start with green */
			|| pixfmt == PIX_FMT_SGRBG16,
		pixfmt != PIX_FMT_SBGGR16		/* blue line */
			&& pixfmt != PIX_FMT_SGBRG16);
}

void bayer_to_bgr48(const uint16_t *bayer,
	uint16_t *bgr, int width, int height, unsigned int pixfmt)
{
	bayer_to_rgbbgr48(bayer, bgr, width, height, pixfmt,
		pixfmt == PIX_FMT_SGBRG16		/* start with green */
			|| pixfmt == PIX_FMT_SGRBG16,
		pixfmt == PIX_FMT_SBGGR16		/* blue line */
			|| pixfmt == PIX_FMT_SGBRG16);
}


static void border_bayer_line_to_bgr96(
	const uint32_t* bayer, const uint32_t* adjacent_bayer,
	uint32_t *bgr, int width, int start_with_green, int blue_line)
{
	uint64_t t0, t1;

	if (start_with_green) {
		/* First pixel */
		if (blue_line) {
			*bgr++ = bayer[1];
			*bgr++ = bayer[0];
			*bgr++ = adjacent_bayer[0];
		} else {
			*bgr++ = adjacent_bayer[0];
			*bgr++ = bayer[0];
			*bgr++ = bayer[1];
		}
		/* Second pixel */
		t0 = ((uint64_t)bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
		t1 = ((uint64_t)adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[1];
			*bgr++ = t0;
			*bgr++ = t1;
		} else {
			*bgr++ = t1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
		}
		bayer++;
		adjacent_bayer++;
		width -= 2;
	} else {
		/* First pixel */
		t0 = ((uint64_t)bayer[1] + adjacent_bayer[0] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[0];
			*bgr++ = t0;
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = t0;
			*bgr++ = bayer[0];
		}
		width--;
	}

	if (blue_line) {
		for ( ; width > 2; width -= 2) {
			t0 = ((uint64_t)bayer[0] + bayer[2] + 1) >> 1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
			bayer++;
			adjacent_bayer++;

			t0 = ((uint64_t)bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
			t1 = ((uint64_t)adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
			*bgr++ = bayer[1];
			*bgr++ = t0;
			*bgr++ = t1;
			bayer++;
			adjacent_bayer++;
		}
	} else {
		for ( ; width > 2; width -= 2) {
			t0 = ((uint64_t)bayer[0] + bayer[2] + 1) >> 1;
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = t0;
			bayer++;
			adjacent_bayer++;

			t0 = ((uint64_t)bayer[0] + bayer[2] + adjacent_bayer[1] + 1) / 3;
			t1 = ((uint64_t)adjacent_bayer[0] + adjacent_bayer[2] + 1) >> 1;
			*bgr++ = t1;
			*bgr++ = t0;
			*bgr++ = bayer[1];
			bayer++;
			adjacent_bayer++;
		}
	}

	if (width == 2) {
		/* Second to last pixel */
		t0 = ((uint64_t)bayer[0] + bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = t0;
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = t0;
		}
		/* Last pixel */
		t0 = ((uint64_t)bayer[1] + adjacent_bayer[2] + 1) >> 1;
		if (blue_line) {
			*bgr++ = bayer[2];
			*bgr++ = t0;
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = t0;
			*bgr++ = bayer[2];
		}
	} else {
		/* Last pixel */
		if (blue_line) {
			*bgr++ = bayer[0];
			*bgr++ = bayer[1];
			*bgr++ = adjacent_bayer[1];
		} else {
			*bgr++ = adjacent_bayer[1];
			*bgr++ = bayer[1];
			*bgr++ = bayer[0];
		}
	}
}

/* From libdc1394, which on turn was based on OpenCV's Bayer decoding */
static void bayer_to_rgbbgr96(const uint32_t *bayer,
	uint32_t *bgr, int width, int height, unsigned int pixfmt,
	int start_with_green, int blue_line)
{
	(void)pixfmt;
	/* render the first line */
	border_bayer_line_to_bgr96(bayer, bayer + width, bgr, width, start_with_green, blue_line);
	bgr += width * 3;

	/* reduce height by 2 because of the special case top/bottom line */
	for (height -= 2; height; height--) {
		uint64_t t0, t1;
		/* (width - 2) because of the border */
		const uint32_t *bayerEnd = bayer + (width - 2);

		if (start_with_green) {
			/* OpenCV has a bug in the next line, which was
			   t0 = (bayer[0] + bayer[width * 2] + 1) >> 1; */
			t0 = ((uint64_t)bayer[1] + bayer[width * 2 + 1] + 1) >> 1;
			/* Write first pixel */
			t1 = ((uint64_t)bayer[0] + bayer[width * 2] + bayer[width + 1] + 1) / 3;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width];
			} else {
				*bgr++ = bayer[width];
				*bgr++ = t1;
				*bgr++ = t0;
			}

			/* Write second pixel */
			t1 = ((uint64_t)bayer[width] + bayer[width + 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
			} else {
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
				*bgr++ = t0;
			}
			bayer++;
		} else {
			/* Write first pixel */
			t0 = ((uint64_t)bayer[0] + bayer[width * 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width];
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = bayer[width];
				*bgr++ = t0;
			}
		}

		if (blue_line) {
			for (; bayer <= bayerEnd - 2; bayer += 2) {
				t0 = ((uint64_t)bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
				t1 = ((uint64_t)bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];

				t0 = ((uint64_t)bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
				t1 = ((uint64_t)bayer[width + 1] + bayer[width + 3] + 1) >> 1;
				*bgr++ = t0;
				*bgr++ = bayer[width + 2];
				*bgr++ = t1;
			}
		} else {
			for (; bayer <= bayerEnd - 2; bayer += 2) {
				t0 = ((uint64_t)bayer[0] + bayer[2] + bayer[width * 2] + bayer[width * 2 + 2] + 2) >> 2;
				t1 = ((uint64_t)bayer[1] + bayer[width] + bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;

				t0 = ((uint64_t)bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
				t1 = ((uint64_t)bayer[width + 1] + bayer[width + 3] + 1) >> 1;
				*bgr++ = t1;
				*bgr++ = bayer[width + 2];
				*bgr++ = t0;
			}
		}

		if (bayer < bayerEnd) {
			/* write second to last pixel */
			t0 = ((uint64_t)bayer[0] + bayer[2] + bayer[width * 2] +
			bayer[width * 2 + 2] + 2) >> 2;
			t1 = ((uint64_t)bayer[1] + bayer[width] +
			bayer[width + 2] + bayer[width * 2 + 1] + 2) >> 2;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;
			}
			/* write last pixel */
			t0 = ((uint64_t)bayer[2] + bayer[width * 2 + 2] + 1) >> 1;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = bayer[width + 2];
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = bayer[width + 2];
				*bgr++ = t0;
			}
			bayer++;
		} else {
			/* write last pixel */
			t0 = ((uint64_t)bayer[0] + bayer[width * 2] + 1) >> 1;
			t1 = ((uint64_t)bayer[1] + bayer[width * 2 + 1] + bayer[width] + 1) / 3;
			if (blue_line) {
				*bgr++ = t0;
				*bgr++ = t1;
				*bgr++ = bayer[width + 1];
			} else {
				*bgr++ = bayer[width + 1];
				*bgr++ = t1;
				*bgr++ = t0;
			}
		}

		/* skip 2 border pixels */
		bayer += 2;

		blue_line = !blue_line;
		start_with_green = !start_with_green;
	}

	/* render the last line */
	border_bayer_line_to_bgr96(bayer + width, bayer, bgr, width, !start_with_green, !blue_line);
}

void bayer_to_rgb96(const uint32_t *bayer,
	uint32_t *bgr, int width, int height, unsigned int pixfmt)
{
	bayer_to_rgbbgr96(bayer, bgr, width, height, pixfmt,
		pixfmt == PIX_FMT_SGBRG32		/* start with green */
			|| pixfmt == PIX_FMT_SGRBG32,
		pixfmt != PIX_FMT_SBGGR32		/* blue line */
			&& pixfmt != PIX_FMT_SGBRG32);
}

void bayer_to_bgr96(const uint32_t *bayer,
	uint32_t *bgr, int width, int height, unsigned int pixfmt)
{
	bayer_to_rgbbgr96(bayer, bgr, width, height, pixfmt,
		pixfmt == PIX_FMT_SGBRG32		/* start with green */
			|| pixfmt == PIX_FMT_SGRBG32,
		pixfmt == PIX_FMT_SBGGR32		/* blue line */
			|| pixfmt == PIX_FMT_SGBRG32);
}
