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

#ifndef ROTATORVIEW_H
#define ROTATORVIEW_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QTimer>

/**
 * A round view of a field rotator, meant to replace the plain dial: it shows
 * where the rotator stands now, where it was told to go, and lets the angle be
 * picked by dragging.
 *
 * Zero is at the top and the angle grows clockwise, the way a position angle
 * reads on the sky. A reversed rotator (ROTATOR_DIRECTION_REVERSED) turns the
 * drawing the other way, so the marker always moves the way the hardware does.
 */
class RotatorView : public QWidget {
	Q_OBJECT

public:
	explicit RotatorView(QWidget *parent = nullptr);

	// ------------------------------------------------------------------
	// Rotator state
	// ------------------------------------------------------------------

	/** ROTATOR_POSITION.POSITION - where the rotator stands, in degrees. */
	void setPosition(double angle);
	double position() const { return m_position; }

	/** Where the rotator was told to go, drawn as a hollow marker. Hidden
	 *  until it is set, and while it sits on the position itself. */
	void setTarget(double angle);
	double target() const { return m_target; }
	void setTargetVisible(bool visible);
	bool isTargetVisible() const { return m_showTarget; }

	/** The travel the rotator allows - the range of ROTATOR_POSITION. A
	 *  picked angle is kept inside it, and anything outside the range is
	 *  drawn as a gap in the scale. The default 0 - 360 is a full turn. */
	void setLimits(double minimum, double maximum);
	double minimum() const { return m_minimum; }
	double maximum() const { return m_maximum; }

	/** ROTATOR_POSITION in the BUSY state - the marker and the rim blink
	 *  pale orange at 1Hz while the rotator turns. */
	void setBusy();
	/** The rotator settled, in the OK state - drawn plain. */
	void setOK();
	bool isBusy() const { return m_busy; }

	/** ROTATOR_DIRECTION_REVERSED - a reversed rotator counts its angles
	 *  the other way round, so the scale is mirrored to match. */
	void setReversed(bool reversed);
	bool isReversed() const { return m_reversed; }

	/** Whether dragging in the view picks an angle. On by default; a
	 *  read only rotator (a ROTATOR_POSITION that is RO) should turn it off. */
	void setPickEnabled(bool enabled);
	bool isPickEnabled() const { return m_pickEnabled; }

	/** Width to height of the detector the rotator carries - the frame in
	 *  the middle is drawn in that shape. 4:3 by default. */
	void setFrameAspect(double aspect);
	double frameAspect() const { return m_frameAspect; }

	// ------------------------------------------------------------------
	// Appearance
	// ------------------------------------------------------------------

	void setTitle(const QString &title);
	QString title() const { return m_title; }

	/** Widget background. Fully transparent by default, so the view shows
	 *  whatever is behind it. Any colour with alpha 0 leaves it unpainted. */
	void setBackgroundColor(const QColor &color);
	QColor backgroundColor() const { return m_backgroundColor; }
	void setRimColor(const QColor &color);
	void setScaleColor(const QColor &color);
	/** The detector frame and the pointer that reads the angle. */
	void setMarkerColor(const QColor &color);
	/** What shows through the detector frame - the sky, as in DomeView. */
	void setSkyColor(const QColor &color);
	void setTargetColor(const QColor &color);
	void setLabelColor(const QColor &color);
	/** Colour the marker and the rim blink in while the rotator turns. */
	void setBusyColor(const QColor &color);

	/** The angle in the middle of the view. */
	void setShowReadout(bool show);
	bool showReadout() const { return m_showReadout; }
	/** The 0/90/180/270 marks around the scale. */
	void setShowLabels(bool show);
	bool showLabels() const { return m_showLabels; }

	/* Square - the view is a circle, so it asks for as much height as it is
	   given width, the same way DomeView does. */
	QSize sizeHint() const override {
		int side = qMax(minimumWidth(), width());
		return QSize(side, side);
	}
	bool hasHeightForWidth() const override { return true; }
	int heightForWidth(int width) const override { return width; }

signals:
	/** The angle was picked in the view, by a click or a drag. The rotator
	 *  is not driven by this - it is the same as typing in the position. */
	void targetPicked(double angle);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

private:
	/** Angle of a point about the middle of the view, in rotator degrees. */
	double angleAt(const QPointF &point) const;
	/** Rotator degrees -> the angle the painter draws at, in degrees. */
	double paintAngle(double angle) const;
	/** Keeps a picked angle inside the travel the rotator allows. */
	double clampToLimits(double angle) const;
	void pickAt(const QPointF &point);

	/** A point at an angle, a fraction of the way out to the rim. */
	QPointF pointAt(const QPointF &middle, double radius, double angle, double fraction) const;
	/** The detector frame, turned to an angle. Outlined when it is the
	 *  target, so that it reads as a place the rotator has yet to reach. */
	void drawFrame(QPainter &painter, const QPointF &middle, double radius,
	               double angle, const QColor &color, bool ghost) const;
	/** The wedge at the rim that reads the angle off the scale. */
	void drawPointer(QPainter &painter, const QPointF &middle, double radius,
	                 double angle, const QColor &color) const;
	/** The target marker, sitting outside the scale so that it cannot be
	 *  taken for the rotator itself. */
	void drawTargetMarker(QPainter &painter, const QPointF &middle, double radius,
	                      double angle, const QColor &color) const;

	double m_position = 0.0;
	double m_target = 0.0;
	bool m_showTarget = false;
	double m_minimum = 0.0;
	double m_maximum = 360.0;
	bool m_busy = false;
	bool m_reversed = false;
	bool m_pickEnabled = true;
	double m_frameAspect = 4.0 / 3.0;

	QString m_title;
	bool m_showReadout = true;
	bool m_showLabels = true;

	/* The DomeView palette, so the two views read as one pair: the structure
	   pale blue-grey, the sky behind it dark, the instrument light, green for
	   where it has to get to and amber while it is on its way. */
	QColor m_backgroundColor = QColor(0, 0, 0, 0);
	QColor m_rimColor = QColor(190, 200, 214);
	QColor m_scaleColor = QColor(190, 195, 205, 115);
	QColor m_markerColor = QColor(188, 191, 198);
	QColor m_skyColor = QColor(24, 44, 68);
	QColor m_targetColor = QColor(80, 200, 120);
	QColor m_labelColor = QColor(190, 195, 205);
	QColor m_busyColor = QColor(226, 186, 108);

	QTimer m_blinkTimer;
	bool m_blinkOn = false;
};

#endif // ROTATORVIEW_H
