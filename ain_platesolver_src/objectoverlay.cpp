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


#include <QBrush>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <algorithm>
#include <cmath>
#include <vector>
#include <indigo/indigocat/indigocat_dso.h>
#include <indigo/indigocat/indigocat_star.h>
#include <objectoverlay.h>

#define MAX_STARS 100
#define MARKER_RADIUS 6
// a deep sky object smaller than this is drawn as a marker, in image pixels
#define MIN_ELLIPSE_PIXELS 16
#define ARCMIN_PER_DEG 60.0
#define DEG_TO_RAD (M_PI / 180.0)
#define LABEL_SIZE 9
// the Tycho-2 stars of the catalogue are listed with ids of their own, the Hipparcos numbers end at 120404 and these start at 131177
#define HIP_ID_LIMIT 130000

static const QColor DSO_COLOR(90, 200, 255);
static const QColor STAR_COLOR(255, 210, 90);

static QPen cosmetic_pen(const QColor &color) {
	QPen pen(color);
	pen.setCosmetic(true);
	pen.setWidth(1);
	return pen;
}

// the overlay must not get in the way of the mouse actions of the image below
static void make_passive(QGraphicsItem *item) {
	item->setAcceptedMouseButtons(Qt::NoButton);
	item->setAcceptHoverEvents(false);
}

ObjectOverlay::ObjectOverlay(QGraphicsItem *image_item) {
	m_root = new QGraphicsRectItem(0, 0, 0, 0, image_item);
	make_passive(m_root);
}

void ObjectOverlay::clear() {
	const QList<QGraphicsItem *> children = m_root->childItems();
	qDeleteAll(children);
}

void ObjectOverlay::add_label(double x, double y, const QColor &color, const QString &label) {
	QGraphicsSimpleTextItem *text = new QGraphicsSimpleTextItem(label, m_root);
	QFont font = text->font();
	font.setPointSize(LABEL_SIZE);
	text->setFont(font);
	text->setBrush(color);
	text->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	text->setPos(x, y);
	make_passive(text);
}

void ObjectOverlay::add_marker(double x, double y, const QColor &color, const QString &label, const QString &tooltip) {
	QGraphicsEllipseItem *marker = new QGraphicsEllipseItem(-MARKER_RADIUS, -MARKER_RADIUS, 2 * MARKER_RADIUS, 2 * MARKER_RADIUS, m_root);
	marker->setPen(cosmetic_pen(color));
	marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	marker->setPos(x, y);
	marker->setToolTip(tooltip);
	make_passive(marker);
	add_label(x, y, color, label);
}

void ObjectOverlay::add_ellipse(double x, double y, double major, double minor, double rotation, const QColor &color, const QString &label, const QString &tooltip) {
	QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-major / 2, -minor / 2, major, minor, m_root);
	ellipse->setPen(cosmetic_pen(color));
	ellipse->setPos(x, y);
	ellipse->setRotation(rotation);
	ellipse->setToolTip(tooltip);
	make_passive(ellipse);
	add_label(x, y, color, label);
}

int ObjectOverlay::show(const preview_image &image, bool jnow) {
	clear();

	double pix_scale = 0, center_ra = 0, center_dec = 0;
	if (image.wcs_data(image.width() / 2.0, image.height() / 2.0, &center_ra, &center_dec, nullptr, nullptr, &pix_scale) != 0) return 0;
	const double width = image.width();
	const double height = image.height();
	const double field_radius = sqrt(width * width + height * height) / 2 * pix_scale;
	const double center_dec_r = center_dec * DEG_TO_RAD;
	int count = 0;

	indigocat_dso_entry *dso = indigocat_get_dso_data();
	for (int i = 0; dso[i].id; i++) {
		double ra = (jnow ? dso[i].ra_now : dso[i].ra) * 15;
		double dec = jnow ? dso[i].dec_now : dso[i].dec;
		double major_deg = dso[i].r1 / ARCMIN_PER_DEG;
		double minor_deg = (dso[i].r2 > 0 ? dso[i].r2 : dso[i].r1) / ARCMIN_PER_DEG;

		// a large object may reach the field with its center far outside
		double reach = field_radius + major_deg / 2;
		double cos_distance = sin(dec * DEG_TO_RAD) * sin(center_dec_r) + cos(dec * DEG_TO_RAD) * cos(center_dec_r) * cos((ra - center_ra) * DEG_TO_RAD);
		if (reach < 90 && cos_distance < cos(reach * DEG_TO_RAD)) continue;

		double x, y;
		if (image.wcs_xy(ra, dec, &x, &y) != 0) continue;
		double major = major_deg / pix_scale;
		double minor = minor_deg / pix_scale;
		if (x < -major / 2 || y < -major / 2 || x > width + major / 2 || y > height + major / 2) continue;

		QString name(dso[i].name ? dso[i].name : "");
		QString label(dso[i].id);
		if (!name.isEmpty()) label += ", " + name;
		QString tooltip = QString("%1\n%2").arg(label, indigocat_dso_type_description[(int)dso[i].type]);
		if (dso[i].mag > 0) tooltip += QString("\nMag %1").arg(dso[i].mag, 0, 'f', 1);
		if (dso[i].r1 > 0) tooltip += QString("\n%1' x %2'").arg(dso[i].r1, 0, 'f', 1).arg(dso[i].r2, 0, 'f', 1);

		if (major < MIN_ELLIPSE_PIXELS) {
			add_marker(x, y, DSO_COLOR, label, tooltip);
		} else {
			// the position angle in the image comes from a point towards it, this also covers the rotation and the parity of the frame
			double angle_r = dso[i].angle * DEG_TO_RAD;
			double step = pix_scale * 10;
			double cos_dec = std::max(cos(dec * DEG_TO_RAD), 1e-6);
			double x_pa, y_pa;
			double rotation = 0;
			if (image.wcs_xy(ra + step * sin(angle_r) / cos_dec, dec + step * cos(angle_r), &x_pa, &y_pa) == 0) {
				rotation = atan2(y_pa - y, x_pa - x) / DEG_TO_RAD;
			}
			add_ellipse(x, y, major, minor, rotation, DSO_COLOR, label, tooltip);
		}
		count++;
	}

	// only the brightest stars of a wide field, labels of all would hide the image
	struct field_star {
		float mag;
		const indigocat_star_entry *entry;
		double x, y;
	};
	std::vector<field_star> stars;
	const double cos_field_radius = field_radius < 90 ? cos(field_radius * DEG_TO_RAD) : -1;
	indigocat_star_entry *star = indigocat_get_star_data();
	for (int i = 0; star[i].hip; i++) {
		double ra = (jnow ? star[i].ra_now : star[i].ra) * 15;
		double dec = jnow ? star[i].dec_now : star[i].dec;
		double cos_distance = sin(dec * DEG_TO_RAD) * sin(center_dec_r) + cos(dec * DEG_TO_RAD) * cos(center_dec_r) * cos((ra - center_ra) * DEG_TO_RAD);
		if (cos_distance < cos_field_radius) continue;
		double x, y;
		if (image.wcs_xy(ra, dec, &x, &y) != 0) continue;
		if (x < 0 || y < 0 || x >= width || y >= height) continue;
		stars.push_back({star[i].mag, star + i, x, y});
	}
	std::sort(stars.begin(), stars.end(), [](const field_star &a, const field_star &b) { return a.mag < b.mag; });
	if (stars.size() > MAX_STARS) stars.resize(MAX_STARS);
	for (const field_star &s : stars) {
		QString catalogue = s.entry->hip < HIP_ID_LIMIT ? QString("HIP %1").arg(s.entry->hip) : QString("Tycho-2 star");
		QString magnitude = QString("%1m").arg(s.mag, 0, 'f', 1);
		QString label = s.entry->name ? QString(s.entry->name) : s.entry->hip < HIP_ID_LIMIT ? catalogue : magnitude;
		add_marker(s.x, s.y, STAR_COLOR, label, QString("%1\n%2\nMag %3").arg(label, catalogue).arg(s.mag, 0, 'f', 2));
		count++;
	}
	return count;
}
