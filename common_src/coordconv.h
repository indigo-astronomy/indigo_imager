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
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

#ifndef _COORDCONV_H
#define _COORDCONV_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* translate real coordinates to telescope coordinates
 *ra and *dec should be real coordinates and will be overwritten with the telescope cooodinates
*/
void real_to_telescope_radec(double telescope_center_ra, double telescope_center_dec, double true_center_ra, double true_center_dec, double *ra, double *dec);

/* derotate xr yr on the image rotated at angle */
int derotate_xy(double xr, double yr, double angle, int parity, double *x, double *y);

// Gnomonic procjection Radius
double gn_R0(double xy_radius, double pix_scale);

// Gnomonic image X Y to Ra Dec
void gn_xy2radec(double x, double y, double x0, double y0, double ra0, double dec0, double R0, double *ra, double *dec);

// Gnomomic Ra Dec to X Y
void gn_radec2xy(double ra, double dec, double ra0, double dec0, double x0, double y0, double R0, double *x, double *y);

#ifdef __cplusplus
}
#endif

#endif /* _COORDCONV_H */
