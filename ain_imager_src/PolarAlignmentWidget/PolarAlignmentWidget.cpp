#include "PolarAlignmentWidget.h"

PolarAlignmentWidget::PolarAlignmentWidget(QWidget *parent) : QWidget(parent) {
	setMinimumSize(200, 200);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	initScaleLevels();
}

void PolarAlignmentWidget::initScaleLevels() {
	m_scaleLevels = {
		{6.0, {1.0, 6.0}},         // 0-6 arcmin: show 1' and 6' circles
		{60.0, {10.0, 60.0}},      // 6-60 arcmin: show 10' and 60' circles
		{300.0, {60.0, 300.0}},    // 1-5 degrees: show 1° and 5° circles
		{900.0, {300.0, 900.0}}    // 5-15 degrees: show 5° and 15° circles
	};
}

void PolarAlignmentWidget::setErrors(double altError, double azError) {
	m_altError = altError;
	m_azError = azError;
	updateScale();
	update();
}

void PolarAlignmentWidget::setMarkerColor(const QColor& color) {
	m_markerColor = color;
	m_useErrorBasedColors = false;
	update();
}

void PolarAlignmentWidget::setMarkerOpacity(qreal opacity) {
	m_markerOpacity = qBound(0.0, opacity, 1.0);
	update();
}

void PolarAlignmentWidget::setBackgroundColor(const QColor& color) {
	m_backgroundColor = color;
	update();
}

void PolarAlignmentWidget::setTargetColor(const QColor& color) {
	m_targetColor = color;
	update();
}

void PolarAlignmentWidget::setLabelColor(const QColor& color)
{
	m_labelColor = color;
	update();
}

void PolarAlignmentWidget::setWidgetOpacity(qreal opacity) {
	m_widgetOpacity = opacity;
	
	// For widgets inside a window, we need to use a different approach
	// Instead of using setWindowOpacity (which affects the window frame too),
	// we'll use setAttribute with WA_TranslucentBackground for the widget itself
	
	if (opacity < 1.0) {
		setAttribute(Qt::WA_TranslucentBackground, true);
	} else {
		setAttribute(Qt::WA_TranslucentBackground, false);
	}
	
	update();
}

void PolarAlignmentWidget::setSmallErrorColor(const QColor& color) {
	m_smallErrorColor = color;
	update();
}

void PolarAlignmentWidget::setMediumErrorColor(const QColor& color) {
	m_mediumErrorColor = color;
	update();
}

void PolarAlignmentWidget::setLargeErrorColor(const QColor& color) {
	m_largeErrorColor = color;
	update();
}

void PolarAlignmentWidget::setUseErrorBasedColors(bool use) {
	m_useErrorBasedColors = use;
	update();
}

void PolarAlignmentWidget::updateScale() {
	double maxError = std::max(std::abs(m_altError), std::abs(m_azError));

	m_currentScaleIndex = 0;
	for (int i = 0; i < m_scaleLevels.size(); ++i) {
		if (maxError <= m_scaleLevels[i].maxError) {
			m_currentScaleIndex = i;
			break;
		}
		if (i == m_scaleLevels.size() - 1) {
			m_currentScaleIndex = i;
		}
	}
}

void PolarAlignmentWidget::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	
	// Set quality rendering for all elements
	painter.setRenderHints(QPainter::Antialiasing |
						QPainter::TextAntialiasing |
						QPainter::SmoothPixmapTransform, true);

	// Apply global opacity to all painting operations
	painter.setOpacity(m_widgetOpacity);

	// Draw background
	painter.fillRect(rect(), m_backgroundColor);

	// Draw the target circles
	drawTarget(painter);

	// Draw the error marker
	drawErrorMarker(painter);
}

void PolarAlignmentWidget::drawTarget(QPainter &painter) {
	QPointF center = getCenter();
	int size = getTargetSize();

	// Draw crosshairs
	painter.save();
	painter.setPen(QPen(m_targetColor, 1, Qt::DashLine));
	painter.drawLine(QPointF(center.x(), center.y() - size/2), 
					QPointF(center.x(), center.y() + size/2));
	painter.drawLine(QPointF(center.x() - size/2, center.y()), 
					QPointF(center.x() + size/2, center.y()));
	painter.restore();

	// Draw the target circles
	painter.save();
	const ScaleLevel &currentScale = m_scaleLevels[m_currentScaleIndex];
	double maxRadius = currentScale.circles.last();

	// Draw center point with transparent ring around it
	painter.save();

	// First draw a transparent circle matching the marker size
	QColor transparentTargetColor = m_targetColor;
	transparentTargetColor.setAlphaF(0.3);
	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(m_targetColor, 1.5));
	painter.drawEllipse(center, 6, 6);

	// Then draw the solid center point
	painter.setBrush(m_targetColor);
	painter.setPen(Qt::NoPen);
	painter.drawEllipse(center, 4, 4);

	painter.restore();

	for (double radius : currentScale.circles) {
		// Convert radius from arcminutes to pixels
		painter.setPen(QPen(m_targetColor, 1.5));
		painter.setBrush(Qt::NoBrush);
		double pixelRadius = (radius / maxRadius) * (size / 2);
		painter.drawEllipse(center, pixelRadius, pixelRadius);

		// Draw labels for this circle
		drawLabels(painter, pixelRadius, radius);
	}
	painter.restore();
}

void PolarAlignmentWidget::drawLabels(QPainter &painter, double pixelRadius, double value) {
	QPointF center = getCenter();
	QString label;

	// Format label based on the value
	if (value >= 60) {
		label = QString("%1°").arg(value / 60.0, 0, 'f', 1);
	} else {
		label = QString("%1'").arg(value, 0, 'f', 0);
	}

	painter.setPen(QPen(m_labelColor, 1.0));
	painter.setRenderHint(QPainter::TextAntialiasing, true);

	QFont font = painter.font();
	font.setPointSize(font.pointSize() + 1);
	painter.setFont(font);

	QFontMetrics fm = painter.fontMetrics();
	QSize textSize = fm.size(Qt::TextSingleLine, label);

	// Position text at the top-right of the circle
	// Use ~40 degrees angle for top-right position (slightly closer to horizontal)
	const double angle = M_PI / 4.5;  // Adjusted angle for better positioning
	const double offsetX = 0.0;       // No offset - place right at the circle edge
	const double offsetY = -1.0;      // Slight vertical adjustment to center better

	QPointF textPos = QPointF(
		center.x() + pixelRadius * cos(angle) + offsetX,
		center.y() - pixelRadius * sin(angle) - textSize.height()/2 + offsetY
	);

	painter.drawText(textPos, label);
}

void PolarAlignmentWidget::drawErrorMarker(QPainter &painter) {
	const double markerSize = 6;
	const double notchSize = 2;
	QPointF markerPos = errorToPoint(m_altError, m_azError);

	QColor markerColor = m_useErrorBasedColors ? getErrorBasedColor() : m_markerColor;

	// Create a semi-transparent color
	QColor markerColorWithAlpha = markerColor;
	markerColorWithAlpha.setAlphaF(m_markerOpacity);

	// Draw the marker as a transparent circle
	painter.save();
	painter.setBrush(QBrush(markerColorWithAlpha));
	painter.setPen(QPen(markerColor, 1.5));
	painter.drawEllipse(markerPos, markerSize, markerSize);

	// Draw horizontal notches
	painter.drawLine(
		markerPos.x() - markerSize - notchSize, markerPos.y(),  // Left notch
		markerPos.x() - markerSize, markerPos.y()
	);
	painter.drawLine(
		markerPos.x() + markerSize + 1, markerPos.y(),      // Right notch
		markerPos.x() + markerSize + 1 + notchSize, markerPos.y()
	);

	// Draw vertical notches (3px long)
	painter.drawLine(
		markerPos.x(), markerPos.y() - markerSize - notchSize,  // Top notch
		markerPos.x(), markerPos.y() - markerSize
	);
	painter.drawLine(
		markerPos.x(), markerPos.y() + markerSize + 1,      // Bottom notch
		markerPos.x(), markerPos.y() + markerSize + 1 + notchSize
	);
	painter.restore();
}

QColor PolarAlignmentWidget::getErrorBasedColor() const {
	double totalError = sqrt(m_altError * m_altError + m_azError * m_azError);

	// Return color based on error magnitude
	if (totalError < 1.0) {
		return m_smallErrorColor;  // < 1 arcmin: Green
	} else if (totalError <= 6.0) {
		return m_mediumErrorColor; // 1-6 arcmin: Orange
	} else {
		return m_largeErrorColor;  // > 6 arcmin: Red
	}
}

QPointF PolarAlignmentWidget::errorToPoint(double altError, double azError) const {
	QPointF center = getCenter();
	int size = getTargetSize();

	double maxRadius = m_scaleLevels[m_currentScaleIndex].circles.last();

	// Convert error from arcminutes to pixels
	double pixelAlt = (altError / maxRadius) * (size / 2);
	double pixelAz = (azError / maxRadius) * (size / 2);

	// In a polar alignment context, typically:
	// - Altitude error is up/down (y-axis, negative is up)
	// - Azimuth error is left/right (x-axis, positive is right)
	return QPointF(center.x() + pixelAz, center.y() - pixelAlt);
}

QPointF PolarAlignmentWidget::getCenter() const {
	return QPointF(width() / 2, height() / 2);
}

int PolarAlignmentWidget::getTargetSize() const {
	return static_cast<int>(std::min(width(), height()) * 0.90);
}

void PolarAlignmentWidget::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	update();
}