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

#include "RotatorView.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFontMetricsF>
#include <QtMath>

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

/* Degrees into 0 - 360. */
static double normalize(double angle) {
	angle = fmod(angle, 360.0);
	if (angle < 0) {
		angle += 360.0;
	}
	return angle;
}

RotatorView::RotatorView(QWidget *parent) : QWidget(parent) {
	setMinimumSize(140, 140);
	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	setSizePolicy(policy);
	setCursor(Qt::PointingHandCursor);

	/* Half a second on, half a second off - one blink a second, as in DomeView. */
	m_blinkTimer.setInterval(500);
	connect(&m_blinkTimer, &QTimer::timeout, this, [this]() {
		m_blinkOn = !m_blinkOn;
		update();
	});
}

// ----------------------------------------------------------------------
// state
// ----------------------------------------------------------------------

void RotatorView::setPosition(double angle) {
	m_position = angle;
	update();
}

void RotatorView::setTarget(double angle) {
	m_target = angle;
	m_showTarget = true;
	update();
}

void RotatorView::setTargetVisible(bool visible) {
	m_showTarget = visible;
	update();
}

void RotatorView::setLimits(double minimum, double maximum) {
	if (maximum > minimum) {
		m_minimum = minimum;
		m_maximum = maximum;
		update();
	}
}

void RotatorView::setBusy() {
	m_busy = true;
	if (!m_blinkTimer.isActive()) {
		m_blinkOn = true;
		m_blinkTimer.start();
	}
	update();
}

void RotatorView::setOK() {
	m_busy = false;
	if (m_blinkTimer.isActive()) {
		m_blinkTimer.stop();
	}
	m_blinkOn = false;
	update();
}

void RotatorView::setReversed(bool reversed) {
	m_reversed = reversed;
	update();
}

void RotatorView::setPickEnabled(bool enabled) {
	m_pickEnabled = enabled;
	setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void RotatorView::setFrameAspect(double aspect) {
	if (aspect > 0) {
		m_frameAspect = aspect;
		update();
	}
}

// ----------------------------------------------------------------------
// appearance
// ----------------------------------------------------------------------

void RotatorView::setTitle(const QString &title) {
	m_title = title;
	update();
}

void RotatorView::setBackgroundColor(const QColor &color) {
	m_backgroundColor = color;
	update();
}

void RotatorView::setRimColor(const QColor &color) {
	m_rimColor = color;
	update();
}

void RotatorView::setScaleColor(const QColor &color) {
	m_scaleColor = color;
	update();
}

void RotatorView::setMarkerColor(const QColor &color) {
	m_markerColor = color;
	update();
}

void RotatorView::setSkyColor(const QColor &color) {
	m_skyColor = color;
	update();
}

void RotatorView::setTargetColor(const QColor &color) {
	m_targetColor = color;
	update();
}

void RotatorView::setLabelColor(const QColor &color) {
	m_labelColor = color;
	update();
}

void RotatorView::setBusyColor(const QColor &color) {
	m_busyColor = color;
	update();
}

void RotatorView::setShowReadout(bool show) {
	m_showReadout = show;
	update();
}

void RotatorView::setShowLabels(bool show) {
	m_showLabels = show;
	update();
}

// ----------------------------------------------------------------------
// angles
// ----------------------------------------------------------------------

/* Qt draws angles counterclockwise from three o'clock, this view counts them
   clockwise from twelve, and a reversed rotator counts them the other way. */
double RotatorView::paintAngle(double angle) const {
	double drawn = m_reversed ? -angle : angle;
	return 90.0 - drawn;
}

double RotatorView::angleAt(const QPointF &point) const {
	QPointF middle(width() / 2.0, height() / 2.0);
	double dx = point.x() - middle.x();
	double dy = middle.y() - point.y();
	double fromTwelve = normalize(90.0 - atan2(dy, dx) * RAD2DEG);
	return normalize(m_reversed ? -fromTwelve : fromTwelve);
}

double RotatorView::clampToLimits(double angle) const {
	/* A rotator that turns all the way round takes any angle. */
	if (m_maximum - m_minimum >= 360.0) {
		return normalize(angle);
	}
	/* The same direction goes by two names, 350 and -10 among them - take
	   the one the travel allows before giving up and clamping. */
	double candidate = normalize(angle);
	if (candidate > m_maximum && candidate - 360.0 >= m_minimum) {
		candidate -= 360.0;
	} else if (candidate < m_minimum && candidate + 360.0 <= m_maximum) {
		candidate += 360.0;
	}
	if (candidate < m_minimum) {
		return m_minimum;
	}
	if (candidate > m_maximum) {
		return m_maximum;
	}
	return candidate;
}

// ----------------------------------------------------------------------
// picking
// ----------------------------------------------------------------------

void RotatorView::pickAt(const QPointF &point) {
	if (!m_pickEnabled) {
		return;
	}
	double angle = clampToLimits(angleAt(point));
	setTarget(angle);
	emit targetPicked(angle);
}

void RotatorView::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		pickAt(event->position());
#else
		pickAt(event->localPos());
#endif
		event->accept();
		return;
	}
	QWidget::mousePressEvent(event);
}

void RotatorView::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() & Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		pickAt(event->position());
#else
		pickAt(event->localPos());
#endif
		event->accept();
		return;
	}
	QWidget::mouseMoveEvent(event);
}

// ----------------------------------------------------------------------
// painting
// ----------------------------------------------------------------------
QPointF RotatorView::pointAt(const QPointF &middle, double radius, double angle, double fraction) const {
	double drawn = paintAngle(angle) * DEG2RAD;
	return QPointF(
		middle.x() + radius * fraction * cos(drawn),
		middle.y() - radius * fraction * sin(drawn)
	);
}

/* The detector the rotator carries, turned to the angle it is held at - the
   frame is what a rotator really does to the image, which a needle never
   showed. The notch marks the top edge, so a turn of 180 degrees still reads. */
void RotatorView::drawFrame(
	QPainter &painter,
	const QPointF &middle,
	double radius,
	double angle,
	const QColor &color,
	bool ghost
) const {
	double halfWidth = radius * 0.47;
	double halfHeight = halfWidth / (m_frameAspect > 0 ? m_frameAspect : 4.0 / 3.0);

	painter.save();
	painter.translate(middle);
	/* Clockwise on screen, and mirrored for a reversed rotator, the same way
	   the scale is. */
	painter.rotate(m_reversed ? -angle : angle);

	QPen pen(color, qMax(1.0, radius * 0.022));
	pen.setJoinStyle(Qt::MiterJoin);
	if (ghost) {
		pen.setStyle(Qt::DashLine);
		pen.setWidthF(qMax(1.0, radius * 0.016));
	}
	painter.setPen(pen);
	if (ghost) {
		painter.setBrush(Qt::NoBrush);
	} else {
		/* The sky seen through the detector, as through the dome opening. */
		QColor sky = m_skyColor;
		sky.setAlphaF(0.92);
		painter.setBrush(sky);
	}
	painter.drawRect(QRectF(-halfWidth, -halfHeight, 2 * halfWidth, 2 * halfHeight));

	/* The notch riding on the top edge. */
	double notch = qMax(4.0, radius * 0.11);
	QPolygonF tab;
	tab << QPointF(-notch, -halfHeight)
	    << QPointF(notch, -halfHeight)
	    << QPointF(0, -halfHeight - notch * 0.9);
	/* Solid on the ghost too - a dashed triangle this small is a scribble. */
	painter.setPen(ghost ? QPen(color, qMax(1.0, radius * 0.016)) : Qt::NoPen);
	painter.setBrush(ghost ? Qt::NoBrush : QBrush(color));
	painter.drawPolygon(tab);

	painter.restore();
}

/* The wedge both the rotator and its target are read by. */
static QPolygonF wedgeAt(
	const QPointF &tip,
	const QPointF &left,
	const QPointF &right
) {
	QPolygonF wedge;
	wedge << tip << left << right;
	return wedge;
}

/* A tapered wedge running into the rim: it reads the angle off the scale far
   more exactly than the frame can. */
void RotatorView::drawPointer(
	QPainter &painter,
	const QPointF &middle,
	double radius,
	double angle,
	const QColor &color
) const {
	painter.setPen(Qt::NoPen);
	painter.setBrush(color);
	painter.drawPolygon(wedgeAt(
		pointAt(middle, radius, angle, 0.985),
		pointAt(middle, radius, angle - 5.0, 0.80),
		pointAt(middle, radius, angle + 5.0, 0.80)
	));
}

/* Where the rotator has to get to, marked just inside the rim in the colour
   DomeView uses for the azimuth the slit has to reach. */
void RotatorView::drawTargetMarker(
	QPainter &painter,
	const QPointF &middle,
	double radius,
	double angle,
	const QColor &color
) const {
	/* The same wedge as the rotator's, left hollow: the shape says "angle on
	   the scale", the outline says "not there yet". */
	QPen pen(color, qMax(1.2, radius * 0.022));
	pen.setJoinStyle(Qt::MiterJoin);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawPolygon(wedgeAt(
		pointAt(middle, radius, angle, 0.985),
		pointAt(middle, radius, angle - 5.0, 0.80),
		pointAt(middle, radius, angle + 5.0, 0.80)
	));
}

void RotatorView::paintEvent(QPaintEvent *event) {
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	if (m_backgroundColor.alpha() > 0) {
		painter.fillRect(rect(), m_backgroundColor);
	}

	QPointF middle(width() / 2.0, height() / 2.0);
	double side = qMin(width(), height());
	double radius = side / 2.0 - qMax(6.0, side * 0.065);
	if (radius <= 4) {
		return;
	}
	QRectF rim(middle.x() - radius, middle.y() - radius, 2 * radius, 2 * radius);

	bool blinking = m_busy && m_blinkOn;
	QColor rimColor = blinking ? m_busyColor : m_rimColor;
	QColor markerColor = blinking ? m_busyColor : m_markerColor;

	/* The scale. What the rotator cannot reach is left as a gap in the rim. */
	QPen rimPen(rimColor, qMax(1.0, radius * 0.022));
	rimPen.setCapStyle(Qt::FlatCap);
	painter.setPen(rimPen);
	painter.setBrush(Qt::NoBrush);
	if (m_maximum - m_minimum >= 360.0) {
		painter.drawEllipse(rim);
	} else {
		/* Qt wants sixteenths of a degree, counterclockwise. */
		double from = paintAngle(m_maximum);
		double span = m_maximum - m_minimum;
		painter.drawArc(rim, qRound(from * 16), qRound((m_reversed ? -span : span) * 16));
	}

	/* A tick every 10 degrees, a longer one every 90. */
	painter.setPen(QPen(m_scaleColor, qMax(0.8, radius * 0.012)));
	for (int tick = 0; tick < 360; tick += 10) {
		double inner = (tick % 90 == 0) ? 0.84 : 0.90;
		painter.drawLine(pointAt(middle, radius, tick, inner), pointAt(middle, radius, tick, 0.965));
	}

	if (m_showLabels) {
		QFont labelFont = font();
		labelFont.setPointSizeF(qMax(6.0, radius * 0.11));
		painter.setFont(labelFont);
		painter.setPen(m_labelColor);
		QFontMetricsF metrics(labelFont);
		const int marks[] = { 0, 90, 180, 270 };
		for (int i = 0; i < 4; i++) {
			QString text = QString::number(marks[i]);
			QPointF centre = pointAt(middle, radius, marks[i], 0.72);
			QRectF box(
				centre.x() - metrics.horizontalAdvance(text) / 2.0,
				centre.y() - metrics.height() / 2.0,
				metrics.horizontalAdvance(text),
				metrics.height()
			);
			painter.drawText(box, Qt::AlignCenter, text);
		}
	}

	/* The rotator itself, and over it where it was told to go - the frame it
	   will end up in and a mark on the scale, both dropped once it is there so
	   that the two do not sit on top of each other. */
	drawFrame(painter, middle, radius, m_position, markerColor, false);

	bool targetApart = fabs(normalize(m_target) - normalize(m_position)) > 0.05;
	if (m_showTarget && targetApart) {
		drawFrame(painter, middle, radius, m_target, m_targetColor, true);
	}

	drawPointer(painter, middle, radius, m_position, markerColor);

	/* Over the rotator's own wedge, so it stays readable where the two meet. */
	if (m_showTarget && targetApart) {
		drawTargetMarker(painter, middle, radius, m_target, m_targetColor);
	}

	/* The angle, upright in the middle of the frame. */
	if (m_showReadout) {
		QFont readoutFont = font();
		readoutFont.setPointSizeF(qMax(7.0, radius * 0.14));
		painter.setFont(readoutFont);
		painter.setPen(m_labelColor);
		QFontMetricsF metrics(readoutFont);
		/* The angle as the rotator reports it, so that it reads the same as
		   the position beside the view. */
		QString text = QString::number(m_position, 'f', 2) + "°";
		QRectF box(
			middle.x() - radius,
			middle.y() - metrics.height() / 2.0,
			2 * radius,
			metrics.height()
		);
		painter.drawText(box, Qt::AlignCenter, text);
	}

	if (!m_title.isEmpty()) {
		QFont titleFont = font();
		titleFont.setPointSizeF(qMax(7.0, radius * 0.14));
		painter.setFont(titleFont);
		painter.setPen(m_labelColor);
		painter.drawText(rect().adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignTop, m_title);
	}
}
