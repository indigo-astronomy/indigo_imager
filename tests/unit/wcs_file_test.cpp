// Unit test of the .wcs file written by Ain Platesolver (ain_platesolver_src/wcsfile.cpp).
//
// The file has to be a valid FITS header, carry the J2000 position, and map pixels to the sky in the
// orientation the application shows under the mouse (coordconv.c). That projection is not the TAN projection
// of the file, so the orientation is compared close to the centre, where the two agree.
#include <cmath>
#include <cstdio>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <coordconv.h>
#include <wcsfile.h>

static int passed = 0;
static int failed = 0;

static void check(bool ok, const QString &description) {
	if (ok) {
		passed++;
	} else {
		failed++;
		printf("  FAIL  %s\n", description.toUtf8().constData());
	}
}

static QMap<QString, QString> parse(const QByteArray &header, bool *valid) {
	QMap<QString, QString> values;
	*valid = header.size() > 0 && header.size() % 2880 == 0;
	bool end = false;
	for (int i = 0; i + 80 <= header.size(); i += 80) {
		const QByteArray card = header.mid(i, 80);
		for (char c : card) {
			if (c < 32 || c > 126) *valid = false;
		}
		if (end) {
			if (card.trimmed().size() > 0) *valid = false;
			continue;
		}
		const QString key = QString::fromLatin1(card.left(8)).trimmed();
		if (key == "END") {
			end = true;
		} else if (card.mid(8, 2) == "= ") {
			values[key] = QString::fromLatin1(card.mid(10)).section('/', 0, 0).trimmed().remove('\'').trimmed();
		}
	}
	if (!end || !header.startsWith("SIMPLE  =                    T")) *valid = false;
	return values;
}

// RA and Dec in degrees of FITS pixel x, y by the TAN projection of the header
static void tan_xy2radec(const QMap<QString, QString> &k, double x, double y, double *ra, double *dec) {
	const double dx = x - k["CRPIX1"].toDouble(), dy = y - k["CRPIX2"].toDouble();
	const double xi = (k["CD1_1"].toDouble() * dx + k["CD1_2"].toDouble() * dy) * DEG_TO_RAD;
	const double eta = (k["CD2_1"].toDouble() * dx + k["CD2_2"].toDouble() * dy) * DEG_TO_RAD;
	const double ra0 = k["CRVAL1"].toDouble() * DEG_TO_RAD, dec0 = k["CRVAL2"].toDouble() * DEG_TO_RAD;
	const double den = cos(dec0) - eta * sin(dec0);
	*ra = fmod((ra0 + atan2(xi, den)) * RAD_TO_DEG + 360, 360);
	*dec = atan2(sin(dec0) + eta * cos(dec0), hypot(xi, den)) * RAD_TO_DEG;
}

// RA and Dec in degrees of image point x, y (0 is the left or top edge) as preview_image::wcs_data() computes them
static void app_xy2radec(const SolverSolution &s, double x, double y, double *ra, double *dec) {
	double dx, dy;
	derotate_xy(x - s.image_width / 2.0, y - s.image_height / 2.0, s.angle, s.parity, &dx, &dy);
	gn_xy2radec(dx, dy, 0, 0, s.ra_j2000 * 15, s.dec_j2000, gn_R0(sqrt(dx * dx + dy * dy), s.scale), ra, dec);
}

static double separation_arcsec(double ra1, double dec1, double ra2, double dec2) {
	ra1 *= DEG_TO_RAD; dec1 *= DEG_TO_RAD; ra2 *= DEG_TO_RAD; dec2 *= DEG_TO_RAD;
	const double a = sin((dec2 - dec1) / 2), b = sin((ra2 - ra1) / 2);
	return 2 * asin(sqrt(a * a + cos(dec1) * cos(dec2) * b * b)) * RAD_TO_DEG * 3600;
}

static SolverSolution solution(double angle, int parity, double dec) {
	SolverSolution s;
	s.ra_j2000 = 5.5;
	s.dec_j2000 = dec;
	s.ra = 5.51;              // JNow, must not reach the file
	s.dec = dec + 0.1;
	s.epoch = 0;
	s.angle = angle;
	s.parity = parity;
	s.scale = 1.5 / 3600;
	s.image_width = 6248;
	s.image_height = 4176;
	return s;
}

static void test_format() {
	bool valid;
	const SolverSolution s = solution(30, -1, 45);
	const QByteArray header = wcs_file_header(s, "M 42 & friends.fits");
	const QMap<QString, QString> k = parse(header, &valid);
	check(valid, "the header is 80 character ASCII cards, starts with SIMPLE, ends with END, padded to 2880 bytes");
	check(header.size() == 2880, "the header is one 2880 byte record");
	check(k["NAXIS"] == "0", "there is no image data");
	check(k["CTYPE1"] == "RA---TAN" && k["CTYPE2"] == "DEC--TAN", "the projection is TAN");
	check(k["WCSAXES"] == "2" && k["EQUINOX"] == "2000.0" && k["RADESYS"] == "ICRS", "the axes and the reference system are set");
	check(fabs(k["CRVAL1"].toDouble() - s.ra_j2000 * 15) < 1e-9 && fabs(k["CRVAL2"].toDouble() - s.dec_j2000) < 1e-9, "CRVAL is the J2000 centre, not the JNow one");
	check(k["CRPIX1"].toDouble() == 3124.5 && k["CRPIX2"].toDouble() == 2088.5, "CRPIX is the centre of the image in FITS pixels");
	check(k["IMAGEW"] == "6248" && k["IMAGEH"] == "4176", "IMAGEW and IMAGEH are the image size");
	check(k["PLTSOLVD"] == "T", "PLTSOLVD is set");
	check(header.contains("COMMENT Solution of M 42 & friends.fits"), "the image name is in a comment");
	const double det = k["CD1_1"].toDouble() * k["CD2_2"].toDouble() - k["CD1_2"].toDouble() * k["CD2_1"].toDouble();
	check(fabs(sqrt(fabs(det)) - s.scale) < 1e-12, "the CD matrix has the pixel scale");
	const QByteArray long_name = wcs_file_header(s, QString(200, 'x') + ".fits");
	check(parse(long_name, &valid).size() == k.size() && valid, "a long image name is cut to the card");
}

// a mirrored image has the opposite sign of the CD determinant
static void test_parity() {
	bool valid;
	const QMap<QString, QString> normal = parse(wcs_file_header(solution(10, -1, 20), "a"), &valid);
	const QMap<QString, QString> mirrored = parse(wcs_file_header(solution(10, 1, 20), "a"), &valid);
	auto det = [](const QMap<QString, QString> &k) {
		return k["CD1_1"].toDouble() * k["CD2_2"].toDouble() - k["CD1_2"].toDouble() * k["CD2_1"].toDouble();
	};
	check(det(normal) * det(mirrored) < 0, "normal and mirrored parity have opposite CD determinants");
}

// every rotation and parity at several declinations: 40 pixels from the centre, in 8 directions, the file and the
// application agree to 0.05"
static void test_orientation() {
	double worst = 0;
	for (int parity : {-1, 1}) {
		for (double dec : {0.0, 45.0, -60.0, 85.0}) {
			for (double angle = 0; angle < 360; angle += 7.5) {
				const SolverSolution s = solution(angle, parity, dec);
				bool valid;
				const QMap<QString, QString> k = parse(wcs_file_header(s, "a"), &valid);
				for (int i = 0; i < 8; i++) {
					const double x = s.image_width / 2.0 + 40 * cos(i * M_PI / 4);
					const double y = s.image_height / 2.0 + 40 * sin(i * M_PI / 4);
					double ra1, dec1, ra2, dec2;
					app_xy2radec(s, x, y, &ra1, &dec1);
					tan_xy2radec(k, x + 0.5, y + 0.5, &ra2, &dec2);   // FITS pixel n covers n - 0.5 to n + 0.5
					const double error = separation_arcsec(ra1, dec1, ra2, dec2);
					if (error > worst) worst = error;
					if (error > 0.05) {
						check(false, QString("angle %1 parity %2 dec %3 direction %4: %5\" off").arg(angle).arg(parity).arg(dec).arg(i).arg(error));
						return;
					}
				}
			}
		}
	}
	check(true, "orientation");
	printf("  orientation: worst difference %.4f\"\n", worst);
}

int main() {
	test_format();
	test_parity();
	test_orientation();
	printf("  wcs_file_test: %d passed, %d failed\n", passed, failed);
	return failed == 0 ? 0 : 1;
}
