#include "PolarAlignmentWidget.h"
#include <QPainterPath>  // Add this include

PolarAlignmentWidget::PolarAlignmentWidget(QWidget *parent) : QWidget(parent) {
	// Increase minimum width to accommodate both the graph and indicators
	setMinimumSize(400, 240); // Wider to ensure space for indicators
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

void PolarAlignmentWidget::setMarkerVisible(bool visible) {
	if (m_showMarker != visible) {
		m_showMarker = visible;
		update();
	}
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
	painter.setRenderHints(
		QPainter::Antialiasing |
		QPainter::TextAntialiasing |
		QPainter::SmoothPixmapTransform, true
	);

	// First draw the rounded rectangle background
	int cornerRadius = 6;
	painter.save();
	// Apply global opacity to the background
	painter.setOpacity(m_widgetOpacity);

	// Create path for rounded rectangle
	QPainterPath path;
	path.addRoundedRect(rect(), cornerRadius, cornerRadius);

	// Set the clip path to ensure all content stays within rounded borders
	painter.setClipPath(path);

	// Draw background
	painter.fillPath(path, m_backgroundColor);

	// Continue with regular painting
	drawTarget(painter);
	drawErrorMarker(painter);
	drawDirectionIndicators(painter);

	painter.restore();
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
		label = QString("%1°").arg(value / 60.0, 0, 'f', 0);
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
	// If marker is hidden, don't draw it
	if (!m_showMarker) {
		return;
	}

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

void PolarAlignmentWidget::drawDirectionIndicators(QPainter &painter) {
	// If marker is hidden, don't draw direction indicators
	if (!m_showMarker) {
		// Draw zeros instead to indicate no errors
		QPointF center = getCenter();
		int size = getTargetSize();

		painter.save();
		int panelX = center.x() + size/2 + 20;
		int centerY = height() / 2;
		int verticalSpacing = 40;
		int altY = centerY + verticalSpacing/2;
		int azY = centerY - verticalSpacing/2;
		int arrowX = panelX + 16;

		QFont valueFont = painter.font();
		valueFont.setPointSize(18);
		valueFont.setBold(false);
		painter.setFont(valueFont);

		// Draw "0'" for both alt and az errors
		QString zeroText = "0.00'";
		painter.setPen(m_labelColor);

		// Get font metrics for vertical alignment
		QFontMetrics fm = painter.fontMetrics();
		int textHeight = fm.height();

		// Draw at the exact same positions as the normal values
		int azTextY = azY + (textHeight/2 - fm.descent());
		int altTextY = altY + (textHeight/2 - fm.descent());

		painter.drawText(arrowX + 12 + 10, azTextY, zeroText);
		painter.drawText(arrowX + 12 + 10, altTextY, zeroText);

		painter.restore();
		return;
	}

	// Original implementation follows...
	QPointF center = getCenter();
	int size = getTargetSize();

	painter.save();

	// Create a panel on the right side
	int panelX = center.x() + size/2 + 20;
	int panelHeight = height() * 0.8;

	// Setup font for values
	QFont valueFont = painter.font();
	valueFont.setPointSize(18);
	valueFont.setBold(false);

	// Get error-based color for arrows only
	QColor errorColor = m_useErrorBasedColors ? getErrorBasedColor() : m_markerColor;

	// Define spacing
	int verticalSpacing = 40;   // Gap between indicators
	int iconSize = 24;          // Larger square for entire arrow
	int textMargin = 10;        // Space between arrow and text

	// Calculate center point for both indicators to ensure perfect symmetry
	int centerY = height() / 2;

	// Swap positions - place Alt at bottom, Az at top for more intuitive mapping
	int altY = centerY + verticalSpacing/2;  // Altitude now at bottom
	int azY = centerY - verticalSpacing/2;   // Azimuth now at top

	int arrowX = panelX + 16;   // Consistent X position for both arrows

	// Get font metrics to calculate text positioning
	painter.setFont(valueFont);
	QFontMetrics fm = painter.fontMetrics();
	int textHeight = fm.height();

	// Draw Azimuth indicator (now at top position)
	if (std::abs(m_azError) > 0.01) {
		painter.setBrush(errorColor);
		painter.setPen(Qt::NoPen);

		QRect iconRect(arrowX - iconSize/2, azY - iconSize/2, iconSize, iconSize);

		if (m_azError < 0) {
			QPolygonF rightArrow;
			// Shaft base
			rightArrow
				<< QPointF(arrowX - iconSize/2, azY - iconSize/6)
				<< QPointF(arrowX - iconSize/2, azY + iconSize/6)
				// Shaft middle
				<< QPointF(arrowX, azY + iconSize/6)
				// Arrow head
				<< QPointF(arrowX, azY + iconSize/3)
				<< QPointF(arrowX + iconSize/2, azY)
				<< QPointF(arrowX, azY - iconSize/3)
				<< QPointF(arrowX, azY - iconSize/6);

			painter.drawPolygon(rightArrow);
		} else {
			QPolygonF leftArrow;
			leftArrow
				<< QPointF(arrowX + iconSize/2, azY - iconSize/6)
				<< QPointF(arrowX + iconSize/2, azY + iconSize/6)
				// Shaft middle
				<< QPointF(arrowX, azY + iconSize/6)
				// Arrow head
				<< QPointF(arrowX, azY + iconSize/3)
				<< QPointF(arrowX - iconSize/2, azY)
				<< QPointF(arrowX, azY - iconSize/3)
				<< QPointF(arrowX, azY - iconSize/6);

			painter.drawPolygon(leftArrow);
		}
	}

	QString azText = formatErrorValue(m_azError);
	painter.setPen(m_labelColor);
	painter.setFont(valueFont);
	int azTextY = azY + (textHeight/2 - fm.descent());
	painter.drawText(arrowX + iconSize/2 + textMargin, azTextY, azText);

	// Draw Altitude indicator (now at bottom position)
	if (std::abs(m_altError) > 0.01) {
		painter.setBrush(errorColor);
		painter.setPen(Qt::NoPen);

		QRect iconRect(arrowX - iconSize/2, altY - iconSize/2, iconSize, iconSize);

		if (m_altError < 0) {
			QPolygonF upArrow;
			// Shaft base
			upArrow
				<< QPointF(arrowX - iconSize/6, altY + iconSize/2)
				<< QPointF(arrowX + iconSize/6, altY + iconSize/2)
				// Shaft middle
				<< QPointF(arrowX + iconSize/6, altY)
				// Arrow head
				<< QPointF(arrowX + iconSize/3, altY)
				<< QPointF(arrowX, altY - iconSize/2)
				<< QPointF(arrowX - iconSize/3, altY)
				<< QPointF(arrowX - iconSize/6, altY);

			painter.drawPolygon(upArrow);
		} else {
			QPolygonF downArrow;
			downArrow
				<< QPointF(arrowX - iconSize/6, altY - iconSize/2)
				<< QPointF(arrowX + iconSize/6, altY - iconSize/2)
				// Shaft middle
				<< QPointF(arrowX + iconSize/6, altY)
				// Arrow head
				<< QPointF(arrowX + iconSize/3, altY)
				<< QPointF(arrowX, altY + iconSize/2)
				<< QPointF(arrowX - iconSize/3, altY)
				<< QPointF(arrowX - iconSize/6, altY);

			painter.drawPolygon(downArrow);
		}
	}

	QString altText = formatErrorValue(m_altError);
	painter.setPen(m_labelColor);
	painter.setFont(valueFont);
	int altTextY = altY + (textHeight/2 - fm.descent());
	painter.drawText(arrowX + iconSize/2 + textMargin, altTextY, altText);

	painter.restore();
}

QString PolarAlignmentWidget::formatErrorValue(double value) const {
	// If marker is hidden, always return "0" with proper formatting
	if (!m_showMarker) {
		return "0.00'";
	}

	QString direction;
	double absValue = std::abs(value);

	if (absValue >= 60) {
		return QString("%1°").arg(absValue / 60.0, 0, 'f', 2);
	} else {
		return QString("%1'").arg(absValue, 0, 'f', 2);
	}
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
	// Calculate available space with consistent padding
	int padding = qMin(width(), height()) * 0.1; // 10% padding

	// Position the center with equal distance from left, top and bottom
	return QPointF(padding + (getTargetSize() / 2), height() / 2);
}

int PolarAlignmentWidget::getTargetSize() const {
	// Calculate available space with consistent padding
	int padding = qMin(width(), height()) * 0.1;

	// Maximum size would be the available height minus padding on both sides
	int maxHeight = height() - (padding * 2);

	// For width, we need to leave space for the indicators on the right
	// Use about 50% of the widget width for the target
	int maxWidth = qMin(static_cast<int>(width() * 0.5), maxHeight);

	return maxWidth;
}

void PolarAlignmentWidget::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	update();
}