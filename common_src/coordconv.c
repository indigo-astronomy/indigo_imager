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

#include <coordconv.h>

static const double DEG2RAD = M_PI / 180.0;
static const double RAD2DEG = 180.0 / M_PI;

void real_to_telescope_radec(double telescope_center_ra, double telescope_center_dec, double true_center_ra, double true_center_dec, double *ra, double *dec) {
	// Transform coordinates
	*ra = *ra - true_center_ra + telescope_center_ra;
	*dec = *dec - true_center_dec + telescope_center_dec;

	//**  Re-normalize coordinates to ensure they are in range
	//  RA
	if (*ra < 0.0)
		*ra += 360.0;
	if (*ra >= 360.0)
		*ra -= 360.0;

	//  DEC
	if (*dec > 90.0) {
		*dec = 180.0 - *dec;
		*ra += 280.0;
		if (*ra >= 360.0)
			*ra -= 360.0;
	}
	if (*dec < -90.0) {
		*dec = -180.0 - *dec;
		*ra += 180.0;
		if (*ra >= 360.0)
			*ra -= 360.0;
	}
}

/* derotate xr yr on the image rotated at angle */
int derotate_xy(double xr, double yr, double angle, int parity, double *x, double *y) {
	double angler = angle;
	if (parity == -1) {
		yr *= -1;
		angler += 180;
	} else if (parity == 0) {
		return -1;
	}

	angler *= DEG2RAD;
	double sin_a = sin(angler);
	double cos_a = cos(angler);
	*x = xr * cos_a - yr * sin_a;
	*y = xr * sin_a + yr * cos_a;
	return 0;
}

// Gnomonic procjection Radius
double gn_R0(double xy_radius, double pix_scale) {
	if (xy_radius <= 0 || pix_scale <= 0) {
		return -1;
	}
	double ang_R = xy_radius * pix_scale / 2 * DEG2RAD;
	return xy_radius * cos(ang_R) / (2 * sin(ang_R));
}

// Gnomonic image X Y to Ra Dec
void gn_xy2radec(double x, double y, double x0, double y0, double ra0, double dec0, double R0, double *ra, double *dec) {
	double sin_ra0 = sin(ra0 * DEG2RAD);
	double cos_ra0 = cos(ra0 * DEG2RAD);

	double sin_dec0 = sin(dec0 * DEG2RAD);
	double cos_dec0 = cos(dec0 * DEG2RAD);

	double dx = x - x0;
	double dy = y - y0;

	double psi = dx * sin_ra0 - dy * sin_dec0 * cos_ra0 + R0 * cos_dec0 * cos_ra0;
	double eta = dy * cos_dec0 + R0 * sin_dec0;
	double ksi = -1 * dx * cos_ra0 - dy * sin_dec0 * sin_ra0 + R0 * cos_dec0 * sin_ra0;

	double sin_dec = eta / sqrt (ksi * ksi + eta * eta + psi * psi);

	*dec  = atan2(sin_dec, sqrt(1 - sin_dec * sin_dec)) * RAD2DEG;
	*ra = atan2(ksi, psi) * RAD2DEG;

	if (*ra < 0) { *ra += 360; }
}

void gn_radec2xy(double ra, double dec, double ra0, double dec0, double x0, double y0, double R0, double *x, double *y) {
	double sin_dec0 = sin(dec0 * DEG2RAD);
	double cos_dec0 = cos(dec0 * DEG2RAD);

	double sin_dec = sin(dec * DEG2RAD);
	double cos_dec = cos(dec * DEG2RAD);

	double dra = (ra - ra0) * DEG2RAD;
	double sin_dra = sin(dra);
	double cos_dra = cos(dra);

	*x = x0 + R0 * (-1 * cos_dec * sin_dra) / (sin_dec * sin_dec0 + cos_dec * cos_dec0 * cos_dra);
	*y = y0 + R0 * (sin_dec * cos_dec0 - cos_dec * sin_dec0 * cos_dra) / (sin_dec * sin_dec0 + cos_dec * cos_dec0 * cos_dra);
}
