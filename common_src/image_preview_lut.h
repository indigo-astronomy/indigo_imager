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

#ifndef _IMAGE_PREVIEW_LUT_H
#define _IMAGE_PREVIEW_LUT_H

typedef enum {
	PREVIEW_STRETCH_NONE = 0,
	PREVIEW_STRETCH_SLIGHT,
	PREVIEW_STRETCH_MODERATE,
	PREVIEW_STRETCH_NORMAL,
	PREVIEW_STRETCH_HARD,
	PREVIEW_STRETCH_COUNT
} preview_stretch_level;

typedef enum {
	COLOR_BALANCE_AUTO = 0,
	COLOR_BALANCE_NONE,
	COLOR_BALANCE_COUNT
} color_balance_t;

typedef struct {
	double clip_black;
	double clip_white;
} preview_stretch_t;

typedef struct {
	uint8_t stretch_level;
	uint8_t balance; /* 0 = AWB, 1 = red, 2 = green, 3 = blue; */
} stretch_config_t;

typedef struct stretch_multiplier {
 	// Stretch algorithm parameter multipliers
 	float shadows;
 	float highlights;
 	float midtones;
 	// The extension parameters are not used.
 	float shadows_expansion;
 	float highlights_expansion;
} stretch_multiplier_t;

static const preview_stretch_t stretch_linear_lut[] = {
	{0.01, 0.001},
	{0.01, 0.07},
	{0.01, 0.25},
	{0.01, 0.75},
	{0.01, 1.30},
};

static const stretch_multiplier_t stretch_multiplier_lut[] = {
	{1.00, 1.00, 0.80, 1.00, 1.00},
	{0.80, 1.00, 0.90, 1.00, 1.00},
	{1.00, 1.00, 1.00, 1.00, 1.00},
	{1.05, 0.95, 0.80, 1.00, 1.00},
	{1.10, 0.90, 0.50, 1.00, 1.00}
};

#endif /* _IMAGE_PREVIEW_LUT_H */
