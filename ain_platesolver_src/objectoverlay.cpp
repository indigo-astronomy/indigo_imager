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
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QPen>
#include <QRegularExpression>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <vector>
#include <indigo/indigocat/indigocat_dso.h>
#include <indigo/indigocat/indigocat_star.h>
#include <antialiaseditems.h>
#include <objectoverlay.h>

#define MAX_STARS 100
#define MARKER_RADIUS 6
// a deep sky object smaller than this is drawn as a marker, in image pixels
#define MIN_ELLIPSE_PIXELS 16
#define ARCMIN_PER_DEG 60.0
#define DEG_TO_RAD (M_PI / 180.0)
#define LABEL_SIZE 9
// in view pixels
#define LABEL_GAP 2
#define LABEL_MARGIN 1
// the Hipparcos numbers of the catalogue end at 120404, above them are the Tycho-2 stars: TYC1 << 17 | TYC2 << 3 | TYC3
#define HIP_ID_LIMIT 130000

static const QColor DSO_COLOR(90, 200, 255);
static const QColor STAR_COLOR(255, 210, 90);

static QPen cosmetic_pen(const QColor &color) {
	QPen pen(color);
	pen.setCosmetic(true);
	pen.setWidth(1);
	return pen;
}

// the labels placed first win: named objects, then the brightest, objects of unknown magnitude last
static double label_rank(bool named, double mag) {
	if (mag <= 0 && !named) return 99;
	return named ? mag - 100 : mag;
}

static QString star_id(int id) {
	if (id < HIP_ID_LIMIT) return QString("HIP %1").arg(id);
	return QString("TYC %1-%2-%3").arg(id >> 17).arg((id >> 3) & 0x3fff).arg(id & 7);
}

// only one id for the label: the Messier number if the object has one, the names stay in the tooltip
static QString dso_label(const QString &id, const QString &name) {
	static const QRegularExpression messier("^M\\d+$");
	const QStringList names = name.split(',');
	for (const QString &part : names) {
		if (messier.match(part.trimmed()).hasMatch()) return part.trimmed();
	}
	return id;
}

// the overlay must not get in the way of the mouse actions of the image below
static void make_passive(QGraphicsItem *item) {
	item->setAcceptedMouseButtons(Qt::NoButton);
	item->setAcceptHoverEvents(false);
}

ObjectOverlay::ObjectOverlay(QGraphicsItem *image_item) {
	m_root = new QGraphicsRectItem(0, 0, 0, 0, image_item);
	// draws nothing, it only clips the overlay to the image area
	m_root->setPen(Qt::NoPen);
	m_root->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
	make_passive(m_root);
}

void ObjectOverlay::clear() {
	const QList<QGraphicsItem *> children = m_root->childItems();
	qDeleteAll(children);
	m_labels.clear();
	m_markers.clear();
}

void ObjectOverlay::add_label(double x, double y, double radius, double rank, const QColor &color, const QString &label) {
	QGraphicsSimpleTextItem *text = new QGraphicsSimpleTextItem(label, m_root);
	QFont font = text->font();
	font.setPointSize(LABEL_SIZE);
	font.setStyleStrategy(QFont::PreferAntialias);
	text->setFont(font);
	text->setBrush(color);
	text->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	make_passive(text);
	m_labels.push_back({text, QPointF(x, y), radius, rank});
}

// The labels keep their size on the screen, so which of them collide depends on the zoom. A label
// goes where it fits inside the image, one that fits nowhere stays hidden until there is room for it.
void ObjectOverlay::layout_labels() {
	double scale = 1;
	if (m_root->scene() && !m_root->scene()->views().isEmpty()) scale = m_root->scene()->views().first()->transform().m11();
	if (scale <= 0) return;

	const QRectF bounds(m_root->rect().topLeft() * scale, m_root->rect().bottomRight() * scale);
	std::vector<QRectF> placed;
	std::vector<QRectF> markers;
	markers.reserve(m_markers.size());
	for (const QPointF &marker : m_markers) {
		markers.push_back(QRectF(marker * scale - QPointF(MARKER_RADIUS, MARKER_RADIUS), QSizeF(2 * MARKER_RADIUS, 2 * MARKER_RADIUS)));
	}

	for (const object_label &label : m_labels) {
		const QPointF anchor = label.anchor * scale;
		const QSizeF size = label.text->boundingRect().size();
		const double w = size.width();
		const double h = size.height();
		const double r = label.radius > 0 ? label.radius + LABEL_GAP : 0;
		const QPointF offsets[] = {
			{r, -h / 2}, {-r - w, -h / 2}, {-w / 2, -r - h}, {-w / 2, r},
			{r * M_SQRT1_2, r * M_SQRT1_2}, {r * M_SQRT1_2, -r * M_SQRT1_2 - h}, {-r * M_SQRT1_2 - w, r * M_SQRT1_2}, {-r * M_SQRT1_2 - w, -r * M_SQRT1_2 - h}
		};

		// clear of the other markers if possible, a label over a marker is still better than none
		bool found = false;
		QRectF rect;
		QPointF offset;
		for (int avoid_markers = 1; avoid_markers >= 0 && !found; avoid_markers--) {
			for (const QPointF &candidate : offsets) {
				rect = QRectF(anchor + candidate, size);
				if (!bounds.contains(rect)) continue;
				const QRectF spaced = rect.adjusted(-LABEL_MARGIN, -LABEL_MARGIN, LABEL_MARGIN, LABEL_MARGIN);
				auto hits = [&spaced](const QRectF &other) { return spaced.intersects(other); };
				if (std::any_of(placed.begin(), placed.end(), hits)) continue;
				if (avoid_markers) {
					bool blocked = false;
					for (size_t i = 0; i < markers.size() && !blocked; i++) {
						blocked = m_markers[i] != label.anchor && spaced.intersects(markers[i]);
					}
					if (blocked) continue;
				}
				offset = candidate;
				found = true;
				break;
			}
		}
		label.text->setVisible(found);
		if (found) {
			label.text->setPos(label.anchor + offset / scale);
			placed.push_back(rect);
		}
	}
}

void ObjectOverlay::add_marker(double x, double y, double rank, const QColor &color, const QString &label, const QString &tooltip) {
	QGraphicsEllipseItem *marker = new AntialiasedEllipseItem(-MARKER_RADIUS, -MARKER_RADIUS, 2 * MARKER_RADIUS, 2 * MARKER_RADIUS, m_root);
	marker->setPen(cosmetic_pen(color));
	marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	marker->setPos(x, y);
	marker->setToolTip(tooltip);
	make_passive(marker);
	m_markers.push_back(QPointF(x, y));
	add_label(x, y, MARKER_RADIUS, rank, color, label);
}

void ObjectOverlay::add_ellipse(double x, double y, double major, double minor, double rotation, double rank, const QColor &color, const QString &label, const QString &tooltip) {
	QGraphicsEllipseItem *ellipse = new AntialiasedEllipseItem(-major / 2, -minor / 2, major, minor, m_root);
	ellipse->setPen(cosmetic_pen(color));
	ellipse->setPos(x, y);
	ellipse->setRotation(rotation);
	ellipse->setToolTip(tooltip);
	make_passive(ellipse);
	// the center of a large object may be outside the image
	const QRectF bounds = m_root->rect();
	add_label(qBound(bounds.left(), x, bounds.right()), qBound(bounds.top(), y, bounds.bottom()), 0, rank, color, label);
}

int ObjectOverlay::show(const preview_image &image, bool jnow) {
	clear();

	double pix_scale = 0, center_ra = 0, center_dec = 0;
	if (image.wcs_data(image.width() / 2.0, image.height() / 2.0, &center_ra, &center_dec, nullptr, nullptr, &pix_scale) != 0) return 0;
	const double width = image.width();
	const double height = image.height();
	m_root->setRect(0, 0, width, height);
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
		QString label = dso_label(dso[i].id, name);
		QString full_name(dso[i].id);
		if (!name.isEmpty()) full_name += ", " + name;
		QString tooltip = QString("%1\n%2").arg(full_name, indigocat_dso_type_description[(int)dso[i].type]);
		if (dso[i].mag > 0) tooltip += QString("\nMag %1").arg(dso[i].mag, 0, 'f', 1);
		if (dso[i].r1 > 0) tooltip += QString("\n%1' x %2'").arg(dso[i].r1, 0, 'f', 1).arg(dso[i].r2, 0, 'f', 1);

		const double rank = label_rank(!name.isEmpty(), dso[i].mag);
		if (major < MIN_ELLIPSE_PIXELS) {
			add_marker(x, y, rank, DSO_COLOR, label, tooltip);
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
			add_ellipse(x, y, major, minor, rotation, rank, DSO_COLOR, label, tooltip);
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
		QString catalogue = star_id(s.entry->hip);
		QString label = s.entry->name ? QString(s.entry->name) : catalogue;
		add_marker(s.x, s.y, label_rank(s.entry->name != nullptr, s.mag), STAR_COLOR, label, QString("%1\n%2\nMag %3").arg(label, catalogue).arg(s.mag, 0, 'f', 2));
		count++;
	}
	std::stable_sort(m_labels.begin(), m_labels.end(), [](const object_label &a, const object_label &b) { return a.rank < b.rank; });
	layout_labels();
	return count;
}
