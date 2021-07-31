// Copyright (c) 2021 Rumen G. Bogdanovski
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

#ifndef _COORDCONV_H
#define _COORDCONV_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  DEG2RAD (M_PI / 180.0)
#define  RAD2DEG (180.0 / M_PI)

typedef struct {
	double x;
	double y;
	double z;
} cartesian_point_t;

typedef struct {
	double a;   /* longitude, az, ra, ha in radians*/
	double d;   /* latitude, alt, dec in radians */
	double r;   /* radius (1 for celestial coordinates) */
} spherical_point_t;

/* convert spherical to cartesian coordinates */
cartesian_point_t indigo_spherical_to_cartesian(const spherical_point_t *spoint);

/* convert spherical (in radians) to cartesian coordinates */
spherical_point_t cartesian_to_sphercal(const cartesian_point_t *cpoint);

/* convert spherical point in radians to ha/ra dec in hours and degrees */
void spherical_to_ra_dec(const spherical_point_t *spoint, const double lst, double *ra, double *dec);

/* convert ha/ra dec in hours and degrees to spherical point in radians */
void ra_dec_to_point(const double ra, const double dec, const double lst, spherical_point_t *spoint);

/* derotate xr yr on the image rotated at angle */
void derotate_xy(double xr, double yr, double angle, double *x, double *y);

#ifdef __cplusplus
}
#endif

#endif /* _COORDCONV_H */
