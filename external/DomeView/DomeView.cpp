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

#include "DomeView.h"

#include <QPainterPath>
#include <QFontMetricsF>
#include <QResizeEvent>

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

DomeView::DomeView(QWidget *parent) : QWidget(parent) {
	setMinimumSize(220, 220);
	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	setSizePolicy(policy);

	/* Half a second on, half a second off - one blink a second. */
	m_blinkTimer.setInterval(500);
	connect(&m_blinkTimer, &QTimer::timeout, this, [this]() {
		m_blinkOn = !m_blinkOn;
		update();
	});
}

// ----------------------------------------------------------------------
// dimensions
// ----------------------------------------------------------------------

void DomeView::setDomeDimensions(
	double radius,
	double shutterWidth,
	double mountPivotOffsetNS,
	double mountPivotOffsetEW,
	double mountPivotVerticalOffset,
	double mountPivotOtaOffset
) {
	m_radius = radius;
	m_shutterWidth = shutterWidth;
	m_pivotNS = mountPivotOffsetNS;
	m_pivotEW = mountPivotOffsetEW;
	m_pivotVertical = mountPivotVerticalOffset;
	m_otaOffset = mountPivotOtaOffset;
	update();
}

void DomeView::setDomeType(DomeType type) {
	m_domeType = type;
	update();
}

void DomeView::setDomeRadius(double radius) {
	m_radius = radius;
	update();
}

void DomeView::setShutterWidth(double width) {
	m_shutterWidth = width;
	update();
}

void DomeView::setMountPivotOffsetNS(double offset) {
	m_pivotNS = offset;
	update();
}

void DomeView::setMountPivotOffsetEW(double offset) {
	m_pivotEW = offset;
	update();
}

void DomeView::setMountPivotVerticalOffset(double offset) {
	m_pivotVertical = offset;
	update();
}

void DomeView::setMountPivotOtaOffset(double offset) {
	m_otaOffset = offset;
	update();
}

void DomeView::setLatitude(double latitude) {
	m_latitude = latitude;
	update();
}

void DomeView::setSideOfPier(SideOfPier side) {
	m_sideOfPier = side;
	update();
}

void DomeView::setTubeLength(double length) {
	m_tubeLength = length;
	update();
}

void DomeView::setApertureDiameter(double diameter) {
	m_apertureDiameter = diameter;
	update();
}

// ----------------------------------------------------------------------
// state
// ----------------------------------------------------------------------

void DomeView::setDomeAzimuth(double azimuth) {
	m_domeAz = normalizeAzimuth(azimuth);
	update();
}

void DomeView::setShutterOpen(bool open) {
	m_shutterPosition = open ? 1.0 : 0.0;
	update();
}

void DomeView::setShutterPosition(double fraction) {
	m_shutterPosition = qBound(0.0, fraction, 1.0);
	update();
}

void DomeView::setShutterBusy() {
	if (!m_shutterBusy) {
		m_shutterBusy = true;
		updateBlink();
	}
}

void DomeView::setShutterOK() {
	if (m_shutterBusy) {
		m_shutterBusy = false;
		updateBlink();
	}
}

void DomeView::setDomeBusy() {
	if (!m_domeBusy) {
		m_domeBusy = true;
		updateBlink();
	}
}

void DomeView::setDomeOK() {
	if (m_domeBusy) {
		m_domeBusy = false;
		updateBlink();
	}
}

/* One blink for the whole widget, so a busy shutter and a turning dome stay in
   step rather than beating against each other. */
void DomeView::updateBlink() {
	bool busy = m_shutterBusy || m_domeBusy;
	if (busy && !m_blinkTimer.isActive()) {
		/* Start lit, so a short move blinks at least once. */
		m_blinkOn = true;
		m_blinkTimer.start();
	} else if (!busy && m_blinkTimer.isActive()) {
		m_blinkOn = false;
		m_blinkTimer.stop();
	}
	update();
}

void DomeView::setTelescopeCoordinates(double azimuth, double altitude) {
	m_scopeAz = normalizeAzimuth(azimuth);
	m_scopeAlt = qBound(-90.0, altitude, 90.0);
	update();
}

void DomeView::setTelescopeAzimuth(double azimuth) {
	m_scopeAz = normalizeAzimuth(azimuth);
	update();
}

void DomeView::setTelescopeAltitude(double altitude) {
	m_scopeAlt = qBound(-90.0, altitude, 90.0);
	update();
}

void DomeView::setTelescopeVisible(bool visible) {
	m_showTelescope = visible;
	update();
}

// ----------------------------------------------------------------------
// appearance
// ----------------------------------------------------------------------

void DomeView::setTitle(const QString &title) {
	m_title = title;
	update();
}

void DomeView::setBackgroundColor(const QColor &color) {
	m_backgroundColor = color;
	update();
}

void DomeView::setDomeColor(const QColor &color) {
	m_domeColor = color;
	update();
}

void DomeView::setOpeningColor(const QColor &color) {
	m_openingColor = color;
	update();
}

void DomeView::setTelescopeColor(const QColor &color) {
	m_telescopeColor = color;
	update();
}

void DomeView::setLabelColor(const QColor &color) {
	m_labelColor = color;
	update();
}

void DomeView::setOkColor(const QColor &color) {
	m_okColor = color;
	update();
}

void DomeView::setBlockedColor(const QColor &color) {
	m_blockedColor = color;
	update();
}

void DomeView::setBusyColor(const QColor &color) {
	m_busyColor = color;
	update();
}

void DomeView::setDomeOpacity(qreal opacity) {
	m_domeOpacity = qBound(0.0, opacity, 1.0);
	update();
}

void DomeView::setShowLabels(bool show) {
	m_showLabels = show;
	update();
}

void DomeView::setShowStatus(bool show) {
	m_showStatus = show;
	update();
}

void DomeView::setShowCompass(bool show) {
	m_showCompass = show;
	update();
}

// ----------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------

double DomeView::normalizeAzimuth(double azimuth) {
	azimuth = fmod(azimuth, 360.0);
	if (azimuth < 0) {
		azimuth += 360.0;
	}
	return azimuth;
}

/* A sensible tube length when the caller did not set one - the OTA offset is
   the distance from the polar axis to the optical axis, so twice that is
   close to the real tube for most mounts. */
double DomeView::effectiveTubeLength() const {
	if (m_tubeLength > 0) {
		return m_tubeLength;
	}
	double length = qMax(2.0 * m_otaOffset, 0.45 * m_radius);
	if (length <= 0) {
		length = 0.5;
	}
	return qMin(length, 1.4 * qMax(m_radius, 0.1));
}

/* Without a diameter from the caller assume something around f/5. */
double DomeView::effectiveApertureDiameter() const {
	if (m_apertureDiameter > 0) {
		return m_apertureDiameter;
	}
	return qMax(0.05, effectiveTubeLength() / 5.0);
}

double DomeView::slitHalfWidth() const {
	if (m_radius <= 0) {
		return 0;
	}
	/* Keep a visible slit even if the driver reports no shutter width. */
	return qBound(0.04 * m_radius, m_shutterWidth / 2.0, 0.98 * m_radius);
}

DomeView::Vec3 DomeView::pointingVector() const {
	double az = m_scopeAz * DEG2RAD;
	double alt = m_scopeAlt * DEG2RAD;
	Vec3 v;
	v.x = cos(alt) * sin(az);
	v.y = cos(alt) * cos(az);
	v.z = sin(alt);
	return v;
}

DomeView::Vec3 DomeView::pivotPoint() const {
	Vec3 p;
	p.x = m_pivotEW;
	p.y = m_pivotNS;
	p.z = m_pivotVertical;
	return p;
}

/* The optical axis of a German equatorial is carried at the end of the
   declination axis, which is perpendicular to both the polar axis and the
   optical axis - that is exactly the model indigo_dome_solve_azimuth() uses.
   The sign of the cross product is the side of the pier the tube is on. */
DomeView::Vec3 DomeView::otaReferencePoint() const {
	Vec3 origin = pivotPoint();
	if (m_otaOffset <= 0) {
		return origin;
	}

	Vec3 v = pointingVector();
	double lat = m_latitude * DEG2RAD;
	Vec3 pole = { 0.0, cos(lat), sin(lat) };

	Vec3 decAxis = {
		pole.y * v.z - pole.z * v.y,
		pole.z * v.x - pole.x * v.z,
		pole.x * v.y - pole.y * v.x
	};
	double norm = sqrt(decAxis.x * decAxis.x + decAxis.y * decAxis.y + decAxis.z * decAxis.z);
	if (norm < 1e-9) {
		/* Pointing along the polar axis - the declination axis can be
		   anywhere in the perpendicular plane, pick the horizontal one. */
		double az = m_scopeAz * DEG2RAD;
		decAxis = { cos(az), -sin(az), 0.0 };
		norm = 1.0;
	}
	decAxis.x /= norm;
	decAxis.y /= norm;
	decAxis.z /= norm;

	/* East or west of the pier is whichever sign carries the tube that way.
	   Where the declination axis has no east west to it - it runs north to
	   south, or straight up - fall back to counterweight down, which is the
	   tube end of the axis being the one above the pivot. */
	double side;
	if (m_sideOfPier != SideOfPierAuto && fabs(decAxis.x) > 1e-6) {
		bool axisRunsEast = decAxis.x > 0;
		bool wantEast = (m_sideOfPier == SideOfPierEast);
		side = (axisRunsEast == wantEast) ? 1.0 : -1.0;
	} else {
		/* On the meridian the axis is level and neither side is lower, so
		   settle it on the east rather than on whichever way rounding went. */
		side = (decAxis.z < -1e-9) ? -1.0 : 1.0;
	}

	origin.x += side * m_otaOffset * decAxis.x;
	origin.y += side * m_otaOffset * decAxis.y;
	origin.z += side * m_otaOffset * decAxis.z;
	return origin;
}

bool DomeView::lineOfSightExit(Vec3 *exit) const {
	if (m_radius <= 0) {
		return false;
	}
	Vec3 origin = otaReferencePoint();
	Vec3 v = pointingVector();

	double originRadius = sqrt(origin.x * origin.x + origin.y * origin.y + origin.z * origin.z);
	if (originRadius >= m_radius) {
		/* The mount does not fit in the dome - nothing sensible to trace. */
		return false;
	}

	double b = origin.x * v.x + origin.y * v.y + origin.z * v.z;
	double c = originRadius * originRadius - m_radius * m_radius;
	double disc = b * b - c;
	if (disc < 0) {
		return false;
	}
	double t = -b + sqrt(disc);
	if (exit) {
		exit->x = origin.x + t * v.x;
		exit->y = origin.y + t * v.y;
		exit->z = origin.z + t * v.z;
	}
	return true;
}

/* Whether the line of sight leaves through the opening. The classic slit is a
   band of the shutter width running from the rim over the zenith - a stadium
   seen from above - and only the part the leaves have uncovered counts. The
   half dome opens a sector about its azimuth, the clamshell a band about its
   own middle. */
bool DomeView::exitIsClear(const Vec3 &exit) const {
	if (m_shutterPosition <= 0) {
		return false;
	}
	double az = m_domeAz * DEG2RAD;
	double along = exit.x * sin(az) + exit.y * cos(az);
	double lateral = exit.x * cos(az) - exit.y * sin(az);

	if (m_domeType == DomeTypeHalfDome) {
		/* Where the exit sits relative to the dome azimuth, -180 to 180. The
		   open sector runs from 90 to the west of it as the shell turns. */
		double relative = atan2(lateral, along) * RAD2DEG;
		return relative <= -90.0 + 180.0 * m_shutterPosition && relative >= -90.0;
	}

	if (m_domeType == DomeTypeClamshell) {
		return fabs(lateral) <= m_radius * m_shutterPosition;
	}

	double halfWidth = slitHalfWidth();
	if (fabs(lateral) > halfWidth * m_shutterPosition) {
		return false;
	}
	if (along >= 0) {
		return true;
	}
	return (along * along + lateral * lateral) <= halfWidth * halfWidth;
}

double DomeView::requiredDomeAzimuth() const {
	Vec3 exit;
	if (!lineOfSightExit(&exit)) {
		return m_scopeAz;
	}
	/* Where the line of sight leaves the dome, whatever the roof is doing -
	   the mark stays put while the shells move. */
	return normalizeAzimuth(atan2(exit.x, exit.y) * RAD2DEG);
}

bool DomeView::isTelescopeBlocked() const {
	Vec3 exit;
	if (!lineOfSightExit(&exit)) {
		return false;
	}
	return !exitIsClear(exit);
}

void DomeView::updateTransform() {
	m_center = QPointF(width() / 2.0, height() / 2.0);

	double half = qMin(width(), height()) / 2.0 - 2.0;
	if (half < 10.0) {
		half = 10.0;
	}

	/* Keep the mount in view even if it is far off the dome center. */
	double extent = qMax(m_radius, 0.1);
	extent = qMax(extent, hypot(m_pivotEW, m_pivotNS) + qMax(0.0, m_otaOffset) + effectiveTubeLength() / 2.0);

	/* Only a classic dome parks anything outside itself. A parked leaf reaches
	   furthest at its outer corner, and that corner swings around as the dome
	   turns - room for exactly that, no more. The wall is a few percent of the
	   radius, near enough here. A half dome and a clamshell keep their shells
	   within the dome, so they get the whole space. */
	if (m_domeType == DomeTypeClassic) {
		extent = qMax(extent, hypot(m_radius + slitHalfWidth() * 0.1, slitHalfWidth() * 2.4));
	}

	/* Two limits at once: everything measured in meters has to fit inside the
	   widget, and the compass labels sit a fixed few pixels outside the rim.
	   Solving both keeps the dome as large as it can be. */
	double byExtent = half / extent;
	double byCompass = (half - (m_showCompass ? 21.0 : 3.0)) / qMax(m_radius, 0.1);
	m_scale = qMax(0.1, qMin(byExtent, byCompass));
}

QPointF DomeView::toScreen(double east, double north) const {
	return QPointF(m_center.x() + east * m_scale, m_center.y() - north * m_scale);
}

// ----------------------------------------------------------------------
// painting
// ----------------------------------------------------------------------

/* Our hint depends on our width, so a width change makes it stale. */
void DomeView::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	if (event->oldSize().width() != width()) {
		updateGeometry();
	}
}

void DomeView::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	painter.setRenderHints(
		QPainter::Antialiasing |
		QPainter::TextAntialiasing |
		QPainter::SmoothPixmapTransform, true
	);

	QPainterPath background;
	background.addRoundedRect(QRectF(rect()), 6, 6);
	painter.setClipPath(background);
	if (m_backgroundColor.alpha() > 0) {
		painter.fillPath(background, m_backgroundColor);
	}

	updateTransform();

	if (m_radius <= 0) {
		painter.setPen(m_labelColor);
		painter.drawText(rect(), Qt::AlignCenter, tr("No dome dimensions"));
		return;
	}

	if (m_showCompass) {
		drawCompass(painter);
	}
	/* The telescope stands under the roof, so it is painted before the shell
	   and shows through it as much as the dome opacity allows. Only what is
	   under the open slit is seen unattenuated. */
	drawOpening(painter);
	drawDomeGuides(painter);
	if (m_showTelescope) {
		drawTelescope(painter);
		drawLineOfSight(painter);
	}
	drawDome(painter);
	drawShutter(painter);
	if (m_showTelescope) {
		drawExitMarkers(painter);
	}
	/* Title and status are gated separately, inside. */
	drawLabels(painter);
}

void DomeView::drawCompass(QPainter &painter) {
	double radius = m_radius * m_scale;
	painter.save();

	QColor tickColor = m_labelColor;
	tickColor.setAlphaF(0.45);
	painter.setPen(QPen(tickColor, 1.0));
	for (int az = 0; az < 360; az += 15) {
		double angle = az * DEG2RAD;
		double dx = sin(angle);
		double dy = -cos(angle);
		double length = (az % 90 == 0) ? 8.0 : ((az % 45 == 0) ? 6.0 : 3.5);
		QPointF from(m_center.x() + dx * (radius + 3.0), m_center.y() + dy * (radius + 3.0));
		QPointF to(m_center.x() + dx * (radius + 3.0 + length), m_center.y() + dy * (radius + 3.0 + length));
		painter.drawLine(from, to);
	}

	QFont font = painter.font();
	font.setPointSizeF(qMax(7.0, font.pointSizeF() - 1.0));
	font.setBold(true);
	painter.setFont(font);
	painter.setPen(m_labelColor);

	QFontMetricsF fm(font);
	const char *labels[4] = { "N", "E", "S", "W" };
	for (int i = 0; i < 4; i++) {
		double angle = i * 90.0 * DEG2RAD;
		double dx = sin(angle);
		double dy = -cos(angle);
		QString text = QString::fromLatin1(labels[i]);
		QPointF at(
			m_center.x() + dx * (radius + 15.0) - fm.horizontalAdvance(text) / 2.0,
			m_center.y() + dy * (radius + 15.0) + fm.capHeight() / 2.0
		);
		painter.drawText(at, text);
	}
	painter.restore();
}

double DomeView::wallWidth() const {
	return qBound(2.5, m_radius * m_scale * 0.05, 9.0);
}

/* A boolean path is only implicitly closed: it fills right, but stroking it
   leaves out the segment back to the start. Rebuild it with every subpath
   closed so the outline is drawn all the way round. */
static QPainterPath strokeable(const QPainterPath &path) {
	QPainterPath closed;
	for (int i = 0; i < path.elementCount(); i++) {
		QPainterPath::Element element = path.elementAt(i);
		if (element.isMoveTo()) {
			if (!closed.isEmpty()) {
				closed.closeSubpath();
			}
			closed.moveTo(element.x, element.y);
		} else if (element.isLineTo()) {
			closed.lineTo(element.x, element.y);
		} else if (element.isCurveTo() && i + 2 < path.elementCount()) {
			closed.cubicTo(
				element.x, element.y,
				path.elementAt(i + 1).x, path.elementAt(i + 1).y,
				path.elementAt(i + 2).x, path.elementAt(i + 2).y
			);
			i += 2;
		}
	}
	if (!closed.isEmpty()) {
		closed.closeSubpath();
	}
	return closed;
}

QTransform DomeView::domeTransform() const {
	QTransform transform;
	transform.translate(m_center.x(), m_center.y());
	transform.rotate(m_domeAz);
	return transform;
}

/* The slit seen from above - a strip of the shutter width running from the rim
   over the zenith, rounded where it passes the zenith and cut off square by
   the dome at the rim. The strip is built long enough that its outer rounding
   falls outside the dome and is clipped away. */
QPainterPath DomeView::localSlitPath() const {
	double radius = m_radius * m_scale;
	double halfWidth = slitHalfWidth() * m_scale;
	double outer = radius + wallWidth() + halfWidth;

	QPainterPath strip;
	strip.addRoundedRect(
		QRectF(-halfWidth, -outer, 2.0 * halfWidth, outer + halfWidth),
		halfWidth, halfWidth
	);
	QPainterPath disc;
	disc.addEllipse(QPointF(0, 0), radius, radius);
	return strip.intersected(disc);
}

QPainterPath DomeView::slitPath() const {
	return domeTransform().map(localSlitPath());
}

/* What the roof has uncovered, in dome coordinates. A classic dome parts its
   leaves from the middle of the slit outwards, a half dome retracts its shell
   into a widening sector, a clamshell folds both shells away from the middle
   and ends up open from rim to rim. */
/* How much smaller the moving half of a half dome is, so it can pass under the
   fixed one. */
#define HALF_DOME_INNER_SCALE 0.965

/* How much of each clamshell shell is left showing when it is fully open. */
#define CLAMSHELL_PARKED_SLIVER 0.07

/* Half a disc, from startAngle counter clockwise, in dome coordinates. */
QPainterPath DomeView::localHalfShell(double radius, double startAngle) const {
	QPainterPath half;
	half.moveTo(0, 0);
	half.arcTo(QRectF(-radius, -radius, 2.0 * radius, 2.0 * radius), startAngle, 180.0);
	half.closeSubpath();
	return half;
}

QPainterPath DomeView::localOpeningPathAt(double position) const {
	if (position <= 0) {
		return QPainterPath();
	}
	double radius = m_radius * m_scale;
	QPainterPath disc;
	disc.addEllipse(QPointF(0, 0), radius, radius);

	if (m_domeType == DomeTypeHalfDome) {
		/* The moving shell turns away from under the fixed one, so the sky
		   comes out from one edge and reaches half the dome. Qt angles run
		   counter clockwise from 3 o'clock and the azimuth is up, so the
		   opening runs from 180 back towards the fixed half. */
		QPainterPath sector;
		sector.moveTo(0, 0);
		sector.arcTo(
			QRectF(-radius, -radius, 2.0 * radius, 2.0 * radius),
			180.0 - 180.0 * position, 180.0 * position
		);
		sector.closeSubpath();
		return sector.intersected(disc);
	}

	if (m_domeType == DomeTypeClamshell) {
		/* A band across the whole dome. It stops just short of the rim so a
		   sliver of each shell stays in view and shows where they are parked -
		   a cue only, the geometry counts a fully open dome as fully open. */
		double gap = radius * (1.0 - CLAMSHELL_PARKED_SLIVER) * position;
		QPainterPath band;
		band.addRect(QRectF(-gap, -radius - 1.0, 2.0 * gap, 2.0 * radius + 2.0));
		return band.intersected(disc);
	}

	double halfWidth = slitHalfWidth() * m_scale;
	double gap = halfWidth * position;
	double reach = radius + wallWidth() + 2.0 * halfWidth;
	QPainterPath band;
	band.addRect(QRectF(-gap, -reach, 2.0 * gap, 2.0 * reach));
	return localSlitPath().intersected(band);
}

QPainterPath DomeView::localOpeningPath() const {
	return localOpeningPathAt(m_shutterPosition);
}

QPainterPath DomeView::openingPath() const {
	return domeTransform().map(localOpeningPath());
}

/* The closed pair covers 40% more than the slit width, so the leaves overlap
   its edges well rather than just meeting them. */
double DomeView::shutterOverlap() const {
	/* Sideways overlap - the ends use their own, see localShutterLeaf(). */
	return slitHalfWidth() * m_scale * 0.4;
}

double DomeView::shutterLeafWidth() const {
	return slitHalfWidth() * m_scale + shutterOverlap();
}

/* One leaf of a two leaf shutter, running the length of the slit. The leaves
   ride on the outside of the dome and this is a view from straight above, so
   nothing occludes them - they are neither cut by the rim nor trimmed to the
   slit, and they overhang both a little. */
QPainterPath DomeView::localShutterLeaf(double fromX) const {
	double radius = m_radius * m_scale;
	double halfWidth = slitHalfWidth() * m_scale;

	/* Closed, the pair overlaps the slit on every side, by 0.4 of the half
	   width sideways, 0.1 out past the rim and 0.2 in past the rounded end
	   over the zenith. The slit itself runs from the rim to one half width
	   past the middle. */
	double outer = radius + halfWidth * 0.1;
	double length = outer + halfWidth * 1.2;

	QPainterPath leaf;
	leaf.addRect(QRectF(fromX, -outer, shutterLeafWidth(), length));
	return leaf;
}

/* The sky seen through the opening, painted before the telescope. */
void DomeView::drawOpening(QPainter &painter) {
	QPainterPath opening = openingPath();
	if (opening.isEmpty()) {
		return;
	}
	QColor sky = m_openingColor;
	sky.setAlphaF(0.92);
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(sky);
	painter.drawPath(opening);
	painter.restore();
}

/* The N-S and E-W lines are drawn on the dome floor, so they belong under
   whatever stands inside it - drawn over the telescope they would make it
   look like glass. */
void DomeView::drawDomeGuides(QPainter &painter) {
	double radius = m_radius * m_scale;
	painter.save();

	/* Not clipped by the slit - they are on the floor, so they carry on
	   through the opening and cross at the middle of the dome. */
	QColor guide = m_domeColor;
	guide.setAlphaF(qMax(0.25, m_domeOpacity * 0.7));
	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(guide, 1.0, Qt::DotLine));
	painter.drawLine(QPointF(m_center.x(), m_center.y() - radius), QPointF(m_center.x(), m_center.y() + radius));
	painter.drawLine(QPointF(m_center.x() - radius, m_center.y()), QPointF(m_center.x() + radius, m_center.y()));

	painter.restore();
}

void DomeView::drawDome(QPainter &painter) {
	double radius = m_radius * m_scale;
	painter.save();

	/* No roof over the part the leaves have uncovered, so whatever is under it
	   stays untouched. */
	QPainterPath opening = openingPath();
	if (!opening.isEmpty()) {
		QPainterPath everything;
		everything.addRect(QRectF(rect()));
		painter.setClipPath(everything.subtracted(opening));
	}

	/* The shell - semi transparent so the mount stays visible through it. */
	QRadialGradient shell(m_center, radius);
	QColor inner = m_domeColor;
	inner.setAlphaF(m_domeOpacity * 0.7);
	QColor outer = m_domeColor;
	outer.setAlphaF(m_domeOpacity);
	shell.setColorAt(0.0, inner);
	shell.setColorAt(1.0, outer);
	painter.setPen(Qt::NoPen);
	painter.setBrush(shell);

	if (m_domeType == DomeTypeHalfDome) {
		/* Two halves: a fixed one and a slightly smaller one that runs round
		   on its own track. The smaller is drawn first so that it disappears
		   under the fixed half as it turns. */
		QColor edge = m_domeColor.darker(200);
		edge.setAlphaF(qMin(1.0, m_domeOpacity + 0.35));

		QPainterPath moving = domeTransform().map(
			localHalfShell(radius * HALF_DOME_INNER_SCALE, -180.0 * m_shutterPosition)
		);
		painter.setPen(QPen(edge, 1.2));
		painter.setBrush(shell);
		painter.drawPath(moving);

		QPainterPath fixed = domeTransform().map(localHalfShell(radius, -180.0));
		painter.setPen(QPen(edge, 1.4));
		painter.setBrush(shell);
		painter.drawPath(fixed);
	} else {
		painter.drawEllipse(m_center, radius, radius);
	}

	painter.restore();
}

void DomeView::drawShutter(QPainter &painter) {
	double radius = m_radius * m_scale;
	double halfWidth = slitHalfWidth() * m_scale;
	double wall = wallWidth();

	/* How far the leaves have parted, in pixels either side of the middle. */
	double gap = halfWidth * m_shutterPosition;

	/* The wall, drawn on top of the shell. Where the shutter has opened the
	   arc skips the gap, so the dome really has a hole in it. */
	QColor wallColor = m_domeColor;
	wallColor.setAlphaF(qMin(1.0, m_domeOpacity + 0.45));
	/* The rotation blinks the wall. So does a moving clamshell shutter once it
	   is fully open, because by then it has no shell left showing to blink. */
	bool wallBlinks = m_domeBusy ||
		(m_shutterBusy && m_domeType == DomeTypeClamshell && m_shutterPosition >= 0.995);
	if (wallBlinks && m_blinkOn) {
		wallColor = m_busyColor;
		wallColor.setAlphaF(qMax(0.8, qMin(1.0, m_domeOpacity + 0.45)));
	}

	painter.save();
	/* A classic dome really has a gap in its wall at the slit, so the rim is
	   clipped against the opening there. A half dome and a clamshell move only
	   their roof, above a wall that stays whole all the way round. */
	if (m_domeType == DomeTypeClassic) {
		QPainterPath opening = openingPath();
		if (!opening.isEmpty()) {
			QPainterPath everything;
			everything.addRect(QRectF(rect()));
			painter.setClipPath(everything.subtracted(opening));
		}
	}
	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(wallColor, wall, Qt::SolidLine, Qt::FlatCap));
	painter.drawEllipse(QRectF(m_center.x() - radius, m_center.y() - radius, 2.0 * radius, 2.0 * radius));
	painter.restore();

	/* The moving parts last of all - they ride on the outside of the dome, so
	   from straight above they lie over everything, wall included. Kept
	   translucent so the opening and the telescope stay readable under them. */
	QColor panel = m_domeColor;
	panel.setAlphaF(qBound(0.2, m_domeOpacity * 0.5 + 0.2, 0.75));
	QColor rib = m_domeColor.darker(165);
	rib.setAlphaF(0.5);
	QColor leafEdge = m_domeColor.darker(230);
	leafEdge.setAlphaF(0.9);

	/* Busy - they take the warm tint on every other blink. */
	if (m_shutterBusy && m_blinkOn) {
		panel = m_busyColor;
		panel.setAlphaF(qMax(0.5, qBound(0.2, m_domeOpacity * 0.5 + 0.2, 0.75)));
		rib = m_busyColor.darker(160);
		rib.setAlphaF(0.55);
		leafEdge = m_busyColor.darker(220);
		leafEdge.setAlphaF(0.95);
	}

	if (m_domeType == DomeTypeClassic) {
		/* Two leaves, their inner edges at the gap, so they slide apart from
		   meeting in the middle of the slit to parked either side of it. */
		double leafWidth = shutterLeafWidth();
		QPainterPath leaves[2] = {
			domeTransform().map(localShutterLeaf(-(gap + leafWidth))),
			domeTransform().map(localShutterLeaf(gap))
		};

		painter.save();
		for (int i = 0; i < 2; i++) {
			painter.setPen(Qt::NoPen);
			painter.setBrush(panel);
			painter.drawPath(leaves[i]);

			if (leafWidth > 7.0) {
				painter.save();
				/* Clip first, while the path and the painter share
				   coordinates, then turn with the dome to lay the ribs
				   across the leaf. */
				painter.setClipPath(leaves[i]);
				painter.translate(m_center);
				painter.rotate(m_domeAz);
				painter.setPen(QPen(rib, 1.0));
				double reach = halfWidth + leafWidth + 2.0;
				for (double y = -radius - wall; y < halfWidth * 1.5; y += qMax(7.0, radius * 0.1)) {
					painter.drawLine(QPointF(-reach, y), QPointF(reach, y));
				}
				painter.restore();
			}

			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(leafEdge, 1.5));
			painter.drawPath(leaves[i]);
		}

		/* Where the two leaves butt together, along the middle of the slit.
		   Only while they still touch. */
		if (gap <= 0.5) {
			painter.translate(m_center);
			painter.rotate(m_domeAz);
			painter.setPen(QPen(leafEdge, 2.0));
			painter.drawLine(
				QPointF(0, -(radius + halfWidth * 0.1)),
				QPointF(0, halfWidth * 1.2)
			);
		}
		painter.restore();
	} else {
		/* A half dome and a clamshell have no separate leaves - the shell
		   itself is the roof and the shading already draws it. Mark where the
		   shells meet when they are shut, and tint the shell while busy. */
		painter.save();
		if (m_shutterBusy && m_blinkOn) {
			/* Only what actually moves takes the tint, but all of it - on a
			   half dome the whole inner shell, wherever it has got to, and on
			   a clamshell both shells. */
			QPainterPath moving;
			if (m_domeType == DomeTypeHalfDome) {
				moving = domeTransform().map(
					localHalfShell(radius * HALF_DOME_INNER_SCALE, -180.0 * m_shutterPosition)
				);
			} else {
				moving.addEllipse(m_center, radius, radius);
				QPainterPath opening = openingPath();
				if (!opening.isEmpty()) {
					moving = moving.subtracted(opening);
				}
			}
			QColor tint = m_busyColor;
			tint.setAlphaF(0.35);
			painter.setPen(Qt::NoPen);
			painter.setBrush(tint);
			painter.drawPath(moving);
		}
		if (m_domeType == DomeTypeClamshell && m_shutterPosition <= 0.005) {
			/* Where the two shells meet - a full diameter. The half dome does
			   not need one, its two halves are drawn with their own edges. */
			painter.translate(m_center);
			painter.rotate(m_domeAz);
			painter.setPen(QPen(leafEdge, 2.0));
			painter.drawLine(QPointF(0, radius), QPointF(0, -radius));
		}
		painter.restore();
	}

	/* The edge of the opening, over everything else. A classic dome shows the
	   whole slit even while it is shut, so it is clear where it will open; the
	   others have no slit to show and just outline what is open. */
	QColor edge = m_openingColor.lighter(320);
	edge.setAlphaF(0.8 + 0.15 * m_shutterPosition);
	painter.save();
	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(edge, 2.0));
	/* Whatever will be uncovered once the roof has finished, not how far it
	   has got so far: the slit of a classic dome, the half a half dome opens,
	   and for a clamshell everything but where its shells come to rest. */
	painter.drawPath(strokeable(domeTransform().map(localOpeningPathAt(1.0))));
	painter.restore();
}

void DomeView::drawTelescope(QPainter &painter) {
	Vec3 pivot = pivotPoint();
	Vec3 ota = otaReferencePoint();
	QPointF pivotAt = toScreen(pivot);
	QPointF otaAt = toScreen(ota);

	double tubeLength = effectiveTubeLength();
	double aperturePx = qMax(3.0, effectiveApertureDiameter() * m_scale);
	/* The tube is a little wider than the optics it carries. */
	double diameterPx = qMax(4.0, aperturePx * 1.15);
	double sinAlt = fabs(sin(m_scopeAlt * DEG2RAD));
	/* A cylinder seen from above: the axis is foreshortened by the altitude
	   while its circular ends open up into ellipses, until at the zenith the
	   tube is exactly its own cross section. */
	double lengthPx = tubeLength * cos(m_scopeAlt * DEG2RAD) * m_scale;
	double capRy = diameterPx / 2.0 * sinAlt;

	painter.save();

	painter.save();
	painter.translate(otaAt);
	painter.rotate(m_scopeAz);

	double half = lengthPx / 2.0;
	QPainterPath silhouette;
	silhouette.setFillRule(Qt::WindingFill);
	silhouette.addRect(QRectF(-diameterPx / 2.0, -half, diameterPx, lengthPx));
	if (capRy > 0.4) {
		/* United one at a time - near the zenith the two ends overlap, and
		   the default odd even rule would punch their overlap out. */
		QPainterPath front;
		front.addEllipse(QPointF(0, -half), diameterPx / 2.0, capRy);
		QPainterPath rear;
		rear.addEllipse(QPointF(0, half), diameterPx / 2.0, capRy);
		silhouette = silhouette.united(front).united(rear);
	}

	/* Shaded across the barrel - dark at both edges with the highlight off
	   center, the way a lit cylinder falls off. */
	QLinearGradient body(-diameterPx / 2.0, 0, diameterPx / 2.0, 0);
	body.setColorAt(0.00, m_telescopeColor.darker(260));
	body.setColorAt(0.16, m_telescopeColor.darker(155));
	body.setColorAt(0.36, m_telescopeColor.lighter(112));
	body.setColorAt(0.60, m_telescopeColor.darker(115));
	body.setColorAt(0.86, m_telescopeColor.darker(195));
	body.setColorAt(1.00, m_telescopeColor.darker(265));
	painter.setPen(QPen(m_telescopeColor.darker(240), 1.0));
	painter.setBrush(body);
	painter.drawPath(silhouette);

	if (capRy > 0.6) {
		/* The back end shows no face - it is barrel curving away from the
		   viewer, so it falls off into shadow. Shaded along the axis across
		   the whole silhouette: filling the rear ellipse instead would draw
		   the edge of its hidden half and make the tube look like glass. */
		QColor shadow = m_telescopeColor.darker(400);
		QColor clear = shadow;
		clear.setAlpha(0);
		QColor deep = shadow;
		deep.setAlpha(215);
		QLinearGradient fall(0, half - capRy * 1.3, 0, half + capRy);
		fall.setColorAt(0.0, clear);
		fall.setColorAt(1.0, deep);
		painter.fillPath(silhouette, fall);

		/* The front end is tilted towards the viewer, so its rim is seen as a
		   flat face around the optics rather than as more barrel. */
		painter.setPen(QPen(m_telescopeColor.darker(235), 1.0));
		painter.setBrush(m_telescopeColor.darker(140));
		painter.drawEllipse(QPointF(0, -half), diameterPx / 2.0, capRy);
	}

	/* The lens or mirror is a disk perpendicular to the optical axis, so it
	   opens up the same way as the rim carrying it. The pointing direction is
	   up in the rotated frame. */
	double apertureRx = aperturePx / 2.0;
	double apertureRy = qMax(0.8, apertureRx * sinAlt);
	painter.setPen(QPen(m_telescopeColor.lighter(170), 1.2));
	painter.setBrush(m_openingColor.lighter(130));
	painter.drawEllipse(QPointF(0, -half), apertureRx, apertureRy);
	painter.restore();

	/* The pivot the telescope turns about - a marker, so it keeps its size
	   whatever tube is mounted on it. */
	double pivotRadius = qBound(3.0, m_radius * m_scale * 0.022, 9.0);
	painter.setPen(QPen(m_telescopeColor.darker(220), 1.2));
	painter.setBrush(m_telescopeColor.darker(260));
	painter.drawEllipse(pivotAt, pivotRadius, pivotRadius);

	painter.restore();
}

void DomeView::drawLineOfSight(QPainter &painter) {
	Vec3 exit;
	if (!lineOfSightExit(&exit)) {
		return;
	}
	bool clear = exitIsClear(exit);
	QColor color = clear ? m_okColor : m_blockedColor;

	/* Start at the aperture rather than at the middle of the tube. */
	Vec3 origin = otaReferencePoint();
	Vec3 v = pointingVector();
	double front = effectiveTubeLength() / 2.0;
	QPointF from = toScreen(origin.x + front * v.x, origin.y + front * v.y);
	QPointF to = toScreen(exit);

	painter.save();
	QPen pen(color, 1.6, Qt::DashLine);
	pen.setDashPattern({ 4, 3 });
	painter.setPen(pen);
	painter.drawLine(from, to);
	painter.restore();
}

/* Both markers sit on the dome shell rather than under it, so they are painted
   after it. */
void DomeView::drawExitMarkers(QPainter &painter) {
	Vec3 exit;
	if (!lineOfSightExit(&exit)) {
		return;
	}
	bool clear = exitIsClear(exit);
	QColor color = clear ? m_okColor : m_blockedColor;

	painter.save();
	painter.setPen(QPen(color, 1.4));
	painter.setBrush(clear ? QColor(color.red(), color.green(), color.blue(), 200) : Qt::NoBrush);
	painter.drawEllipse(toScreen(exit), 3.5, 3.5);

	/* Where the slit has to be - the azimuth of the exit point, marked just
	   inside the wall. */
	double radius = m_radius * m_scale;
	double angle = requiredDomeAzimuth() * DEG2RAD;
	double dx = sin(angle);
	double dy = -cos(angle);
	painter.setPen(QPen(color, 2.5, Qt::SolidLine, Qt::FlatCap));
	painter.drawLine(
		QPointF(m_center.x() + dx * radius * 0.88, m_center.y() + dy * radius * 0.88),
		QPointF(m_center.x() + dx * radius * 0.99, m_center.y() + dy * radius * 0.99)
	);
	painter.restore();
}

/* Only the title and the line saying whether the telescope can see out - the
   numbers behind them are the caller's to show, and it has the property values
   already. */
void DomeView::drawLabels(QPainter &painter) {
	painter.save();

	QFont font = painter.font();
	font.setPointSizeF(qMax(7.5, font.pointSizeF() - 1.0));
	painter.setFont(font);
	QFontMetricsF fm(font);
	const double pad = 6.0;

	if (m_showLabels && !m_title.isEmpty()) {
		QFont titleFont = font;
		titleFont.setBold(true);
		painter.setFont(titleFont);
		painter.setPen(m_labelColor);
		painter.drawText(QPointF(pad, pad + fm.ascent()), m_title);
		painter.setFont(font);
	}

	if (m_showStatus && m_showTelescope) {
		Vec3 exit;
		QString status;
		QColor statusColor;
		if (!lineOfSightExit(&exit)) {
			status = tr("Mount outside the dome");
			statusColor = m_blockedColor;
		} else if (m_shutterPosition <= 0) {
			status = (m_domeType == DomeTypeClassic) ? tr("Shutter closed") : tr("Dome closed");
			statusColor = m_blockedColor;
		} else if (exitIsClear(exit)) {
			status = (m_domeType == DomeTypeClassic) ? tr("Slit aligned") : tr("View is clear");
			statusColor = m_okColor;
		} else if (m_domeType == DomeTypeClamshell) {
			/* Turning it would not help, the shells part where they part. */
			status = tr("Shells block the view");
			statusColor = m_blockedColor;
		} else {
			status = tr("Dome blocks the view, needs %1°").arg(requiredDomeAzimuth(), 0, 'f', 1);
			statusColor = m_blockedColor;
		}
		painter.setPen(statusColor);
		painter.drawText(QPointF(pad, height() - pad - fm.descent()), status);
	}
	painter.restore();
}
