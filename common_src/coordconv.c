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

#include<coordconv.h>

/* convert spherical to cartesian coordinates */
cartesian_point_t spherical_to_cartesian(const spherical_point_t *spoint) {
	cartesian_point_t cpoint = {0,0,0};
	double cos_d = cos(spoint->d);
	cpoint.x = spoint->r * cos_d * cos(spoint->a);
	cpoint.y = spoint->r * cos_d * sin(spoint->a);
	cpoint.z = spoint->r * sin(spoint->d);
	return cpoint;
}

/* convert spherical (in radians) to cartesian coordinates */
spherical_point_t cartesian_to_sphercal(const cartesian_point_t *cpoint) {
	spherical_point_t spoint = {0,0,0};
	spoint.r = sqrt(cpoint->x * cpoint->x + cpoint->y * cpoint->y + cpoint->z * cpoint->z);
	spoint.a = atan(cpoint->y / cpoint->x);
	spoint.d = acos(cpoint->z / spoint.r);
	return spoint;
}

/* convert spherical point in radians to ha/ra dec in hours and degrees */
void spherical_to_ra_dec(const spherical_point_t *spoint, const double lst, double *ra, double *dec) {
	*ra  = lst + spoint->a / DEG2RAD / 15.0 ;
	if (*ra > 24) {
		*ra -= 24.0;
	}
	*dec = spoint->d / DEG2RAD;
}

/* convert ha/ra dec in hours and degrees to spherical point in radians */
void ra_dec_to_point(const double ra, const double dec, const double lst, spherical_point_t *spoint) {
	double ha = lst - ra;
	if (ha < 0) {
		ha += 24;
	}
	spoint->a = ha * 15.0 * DEG2RAD;
	spoint->d = dec * DEG2RAD;
	spoint->r = 1;
}

/* derotate xr yr on the image rotated at angle */
void derotate_xy(double xr, double yr, double angle, double *x, double *y) {
	double angler = angle * DEG2RAD;
	double sin_a = sin(angler);
	double cos_a = cos(angler);
	*x = xr * cos_a - yr * sin_a;
	*y = xr * sin_a + yr * cos_a;
}
