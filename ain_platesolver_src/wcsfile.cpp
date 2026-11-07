// Copyright (c) 2026 Rumen G.Bogdanovski
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


#include <cmath>
#include <QDateTime>
#include <wcsfile.h>

// One 80 character FITS header card: strings start in column 11, other values end in column 30
static QByteArray fits_card(const QString &key, const QString &value = QString(), const QString &comment = QString()) {
	QString card = key.leftJustified(8, ' ');
	if (!value.isNull()) {
		card += "= " + (value.startsWith('\'') ? value.leftJustified(20, ' ') : value.rightJustified(20, ' '));
	}
	if (!comment.isEmpty()) {
		card += " / " + comment;
	}
	return card.leftJustified(80, ' ', true).toLatin1();
}

static QString fits_number(double value) {
	return QString::number(value, 'G', 12);
}

// The TAN projection of the solution: x grows to the right and y down the image as stored, i.e. as FITS pixel
// coordinates of a FITS image and as the rows of other formats. The rotation and parity follow derotate_xy() and
// gn_xy2radec() of coordconv.c used for the coordinates under the mouse: x maps to -RA and y to +Dec before the rotation.
QByteArray wcs_file_header(const SolverSolution &solution, const QString &image_name) {
	const double angle = (solution.angle + (solution.parity == -1 ? 180 : 0)) * M_PI / 180;
	const double flip = solution.parity == -1 ? -1 : 1;
	const double scale = solution.scale;
	QByteArray header;
	header += fits_card("SIMPLE", "T", "file does conform to FITS standard");
	header += fits_card("BITPIX", "8", "no image data, the WCS header only");
	header += fits_card("NAXIS", "0");
	header += fits_card("WCSAXES", "2", "number of WCS axes");
	header += fits_card("CTYPE1", "'RA---TAN'", "TAN (gnomonic) projection");
	header += fits_card("CTYPE2", "'DEC--TAN'", "TAN (gnomonic) projection");
	header += fits_card("CUNIT1", "'deg'", "unit of CRVAL1 and CD1_n");
	header += fits_card("CUNIT2", "'deg'", "unit of CRVAL2 and CD2_n");
	header += fits_card("RADESYS", "'ICRS'", "reference system of the catalogs");
	header += fits_card("EQUINOX", "2000.0", "equinox of the coordinates");
	header += fits_card("CRVAL1", fits_number(solution.ra_j2000 * 15), "RA of the reference pixel, degrees");
	header += fits_card("CRVAL2", fits_number(solution.dec_j2000), "Dec of the reference pixel, degrees");
	header += fits_card("CRPIX1", fits_number((solution.image_width + 1) / 2.0), "X of the reference pixel, the image center");
	header += fits_card("CRPIX2", fits_number((solution.image_height + 1) / 2.0), "Y of the reference pixel, the image center");
	header += fits_card("CD1_1", fits_number(-scale * cos(angle)), "transformation matrix, degrees per pixel");
	header += fits_card("CD1_2", fits_number(scale * sin(angle) * flip), "transformation matrix, degrees per pixel");
	header += fits_card("CD2_1", fits_number(scale * sin(angle)), "transformation matrix, degrees per pixel");
	header += fits_card("CD2_2", fits_number(scale * cos(angle) * flip), "transformation matrix, degrees per pixel");
	header += fits_card("IMAGEW", QString::number(solution.image_width), "image width, pixels");
	header += fits_card("IMAGEH", QString::number(solution.image_height), "image height, pixels");
	header += fits_card("PLTSOLVD", "T", "plate solved");
	header += fits_card("DATE", "'" + QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ss") + "'", "creation date of this file, UTC");
	header += fits_card("CREATOR", "'Ain Platesolver'", "software that solved the image");
	header += fits_card("COMMENT Solution of " + image_name);
	header += fits_card("END");
	header.append(QByteArray((2880 - header.size() % 2880) % 2880, ' '));
	return header;
}
