// Copyright (c) 2025 Rumen G.Bogdanovski
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

#pragma once

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>
#include <cmath>

class PolarAlignmentWidget : public QWidget {
	Q_OBJECT

public:
	explicit PolarAlignmentWidget(QWidget *parent = nullptr);

	void setErrors(double altError, double azError);

	double getAltError() const { return m_altError; }
	double getAzError() const { return m_azError; }

	void setMarkerColor(const QColor& color);
	void setMarkerOpacity(qreal opacity);
	QColor getMarkerColor() const { return m_markerColor; }
	qreal getMarkerOpacity() const { return m_markerOpacity; }

	void setSmallErrorColor(const QColor& color);
	void setMediumErrorColor(const QColor& color);
	void setLargeErrorColor(const QColor& color);
	void setUseErrorBasedColors(bool use);

	void setBackgroundColor(const QColor& color);
	void setTargetColor(const QColor& color);
	void setLabelColor(const QColor& color);
	void setWidgetOpacity(qreal opacity);
	QColor getBackgroundColor() const { return m_backgroundColor; }
	QColor getTargetColor() const { return m_targetColor; }
	QColor getLabelColor() const { return m_labelColor; }
	qreal getWidgetOpacity() const { return m_widgetOpacity; }

	void setMarkerVisible(bool visible);
	bool isMarkerVisible() const { return m_showMarker; }

	void setTitle(const QString& title);
	QString getTitle() const { return m_title; }

	// Warning state methods
	void setWarning(bool warning);
	bool hasWarning() const { return m_showWarning; }

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private:
	int m_valueFontSize = 24;  // Font size for values
	int m_titleFontSize = 12;  // Font size for title
	double m_altError = 0.0;  // Altitude error in arcminutes
	double m_azError = 0.0;   // Azimuth error in arcminutes
	double m_totalError = 0.0; // Total error in arcminutes

	// Marker appearance properties
	QColor m_markerColor = Qt::red;
	QColor m_smallErrorColor = Qt::green;
	QColor m_mediumErrorColor = QColor(255, 180, 0);
	QColor m_largeErrorColor = Qt::red;
	bool m_useErrorBasedColors = true;
	qreal m_markerOpacity = 0.35;

	// Widget appearance properties
	QColor m_backgroundColor = Qt::black;
	QColor m_targetColor = Qt::lightGray;
	QColor m_labelColor = Qt::white;
	qreal m_widgetOpacity = 1.0;

	QString m_title = "Polar Error";

	// Scale levels in arcminutes
	struct ScaleLevel {
		double maxError;	 // Maximum error for this scale level (in arcminutes)
		QVector<double> circles;  // Circle radii to show (in arcminutes)
	};

	QVector<ScaleLevel> m_scaleLevels;

	int m_currentScaleIndex = 0;

	bool m_showMarker = true;  // Visibility state of the marker and arrows
	bool m_showWarning = false;  // Flag to show warning triangle

	void initScaleLevels();
	void updateScale();
	void drawTarget(QPainter &painter);
	void drawErrorMarker(QPainter &painter);
	void drawLabels(QPainter &painter, double radius, double value);
	void drawTitle(QPainter &painter);  // Add drawing method for title

	QPointF errorToPoint(double altError, double azError) const;
	
	int getTargetSize() const;
	QPointF getCenter() const;

	QColor getErrorBasedColor() const;

	void drawDirectionIndicators(QPainter &painter);
	QString formatErrorValue(double value) const;

	void drawWarning(QPainter &painter);
};