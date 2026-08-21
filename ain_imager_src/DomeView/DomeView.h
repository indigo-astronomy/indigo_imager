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

#pragma once

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QPainterPath>
#include <QTransform>
#include <QColor>
#include <QString>
#include <cmath>

/**
 * DomeView - a top (bird's eye) view of a dome with the telescope inside it.
 *
 * The dome is drawn semi transparent so the mount and the optical tube stay
 * visible, the shutter (slit) rotates with the dome azimuth and is drawn open
 * or closed, and the telescope is drawn at its azimuth with the tube
 * foreshortened by its altitude - at the zenith the tube is seen end on.
 *
 * The line of sight is traced from the optical axis reference point to the
 * dome surface, so it is immediately visible whether the slit lets the
 * telescope see out or the dome wall blocks it.
 *
 * All dimensions are in meters and match the items of the INDIGO
 * DOME_DIMENSION property. Azimuth is in degrees, N = 0, E = 90, S = 180,
 * W = 270, altitude is in degrees above the horizon.
 *
 * The widget is plain Qt - it does not depend on the INDIGO headers, the
 * caller feeds it the property values.
 */
class DomeView : public QWidget {
	Q_OBJECT

public:
	/** Which side of the pier the optical tube sits on - the values of
	 *  MOUNT_SIDE_OF_PIER, plus letting the widget work it out itself. */
	enum SideOfPier {
		SideOfPierAuto = 0,  /**< derived from the pointing, counterweight down */
		SideOfPierEast,      /**< MOUNT_SIDE_OF_PIER.EAST - tube east of the pier */
		SideOfPierWest       /**< MOUNT_SIDE_OF_PIER.WEST - tube west of the pier */
	};

	explicit DomeView(QWidget *parent = nullptr);

	// ------------------------------------------------------------------
	// Dome dimensions - DOME_DIMENSION property items, all in meters
	// ------------------------------------------------------------------

	/** Set all DOME_DIMENSION items at once. */
	void setDomeDimensions(
		double radius,
		double shutterWidth,
		double mountPivotOffsetNS = 0,
		double mountPivotOffsetEW = 0,
		double mountPivotVerticalOffset = 0,
		double mountPivotOtaOffset = 0
	);

	void setDomeRadius(double radius);                    /**< DOME_DIMENSION.RADIUS */
	void setShutterWidth(double width);                   /**< DOME_DIMENSION.SHUTTER_WIDTH */
	void setMountPivotOffsetNS(double offset);            /**< DOME_DIMENSION.MOUNT_PIVOT_OFFSET_NS (+N/-S) */
	void setMountPivotOffsetEW(double offset);            /**< DOME_DIMENSION.MOUNT_PIVOT_OFFSET_EW (+E/-W) */
	void setMountPivotVerticalOffset(double offset);      /**< DOME_DIMENSION.MOUNT_PIVOT_VERTICAL_OFFSET */
	void setMountPivotOtaOffset(double offset);           /**< DOME_DIMENSION.MOUNT_PIVOT_OTA_OFFSET */

	double domeRadius() const { return m_radius; }
	double shutterWidth() const { return m_shutterWidth; }
	double mountPivotOffsetNS() const { return m_pivotNS; }
	double mountPivotOffsetEW() const { return m_pivotEW; }
	double mountPivotVerticalOffset() const { return m_pivotVertical; }
	double mountPivotOtaOffset() const { return m_otaOffset; }

	/** Site latitude in degrees (+N). Only used to orient the declination
	 *  axis, so that the tube is offset from the pier the way the real
	 *  German equatorial mount does. */
	void setLatitude(double latitude);
	double latitude() const { return m_latitude; }

	/** MOUNT_SIDE_OF_PIER - which side of the pier carries the tube. Auto
	 *  keeps the counterweight down. Where the declination axis runs plain
	 *  north to south it has no east or west to it, and auto is used. */
	void setSideOfPier(SideOfPier side);
	SideOfPier sideOfPier() const { return m_sideOfPier; }

	/** Optical tube length in meters. Zero (the default) derives a
	 *  plausible length from the dome radius and the OTA offset. */
	void setTubeLength(double length);
	double tubeLength() const { return m_tubeLength; }

	/** Lens or mirror diameter in meters - the tube is drawn a little wider
	 *  than that. Zero (the default) derives it from the tube length. */
	void setApertureDiameter(double diameter);
	double apertureDiameter() const { return m_apertureDiameter; }

	// ------------------------------------------------------------------
	// Dome state
	// ------------------------------------------------------------------

	/** DOME_HORIZONTAL_COORDINATES.AZ - the dome rotates to this azimuth. */
	void setDomeAzimuth(double azimuth);
	double domeAzimuth() const { return m_domeAz; }

	/** The rotation in the BUSY state - the dome wall, the outer circle,
	 *  blinks pale orange at 1Hz. Independent of the azimuth itself. */
	void setDomeBusy();
	/** The rotation settled, in the OK state - the wall is drawn plain. */
	void setDomeOK();
	bool isDomeBusy() const { return m_domeBusy; }

	/** DOME_SHUTTER - true for OPENED, false for CLOSED. The same as driving
	 *  the shutter position to 1 or to 0. */
	void setShutterOpen(bool open);
	/** True once the shutter is fully open. Use shutterPosition() to tell a
	 *  half open shutter from a closed one. */
	bool isShutterOpen() const { return m_shutterPosition >= 1.0; }

	/** How far the shutter has travelled: 0 closed, 1 fully open. The leaves
	 *  part gradually and only the gap between them lets the telescope out. */
	void setShutterPosition(double fraction);
	double shutterPosition() const { return m_shutterPosition; }

	/** DOME_SHUTTER in the BUSY state - the leaves blink pale orange at 1Hz
	 *  while they are moving. */
	void setShutterBusy();
	/** DOME_SHUTTER settled, in the OK state - the leaves are drawn plain. */
	void setShutterOK();
	bool isShutterBusy() const { return m_shutterBusy; }

	// ------------------------------------------------------------------
	// Telescope state
	// ------------------------------------------------------------------

	/** MOUNT_HORIZONTAL_COORDINATES - where the telescope points. */
	void setTelescopeCoordinates(double azimuth, double altitude);
	void setTelescopeAzimuth(double azimuth);
	void setTelescopeAltitude(double altitude);
	double telescopeAzimuth() const { return m_scopeAz; }
	double telescopeAltitude() const { return m_scopeAlt; }

	void setTelescopeVisible(bool visible);
	bool isTelescopeVisible() const { return m_showTelescope; }

	// ------------------------------------------------------------------
	// Derived state
	// ------------------------------------------------------------------

	/** Dome azimuth that puts the slit on the line of sight, in degrees. */
	double requiredDomeAzimuth() const;

	/** True when the dome wall or the closed shutter blocks the view. */
	bool isTelescopeBlocked() const;

	// ------------------------------------------------------------------
	// Appearance
	// ------------------------------------------------------------------

	void setTitle(const QString &title);
	QString title() const { return m_title; }

	void setBackgroundColor(const QColor &color);
	void setDomeColor(const QColor &color);
	void setOpeningColor(const QColor &color);
	void setTelescopeColor(const QColor &color);
	void setLabelColor(const QColor &color);
	void setOkColor(const QColor &color);
	void setBlockedColor(const QColor &color);
	/** Colour the shutter blinks in while it is busy. */
	void setBusyColor(const QColor &color);

	/** Opacity of the dome shell, 0 - 1. */
	void setDomeOpacity(qreal opacity);
	qreal domeOpacity() const { return m_domeOpacity; }

	/** The title in the top left corner. */
	void setShowLabels(bool show);
	bool showLabels() const { return m_showLabels; }

	/** The line in the bottom left corner saying whether the telescope can
	 *  see out through the slit. */
	void setShowStatus(bool show);
	bool showStatus() const { return m_showStatus; }

	void setShowCompass(bool show);
	bool showCompass() const { return m_showCompass; }

	/* Square - the view is a circle, so it asks for as much height as it is
	   given width. The hint follows the width we were given and a resize asks
	   the layout to look again, because heightForWidth() on its own is
	   ignored by a layout aligned to the top: that hands out hint heights and
	   never asks. */
	QSize sizeHint() const override {
		int side = qMax(minimumWidth(), width());
		return QSize(side, side);
	}
	bool hasHeightForWidth() const override { return true; }
	int heightForWidth(int width) const override { return width; }

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private:
	/** A vector in dome coordinates: x = East, y = North, z = up, in meters. */
	struct Vec3 {
		double x = 0;
		double y = 0;
		double z = 0;
	};

	// geometry
	double m_radius = 2.5;
	double m_shutterWidth = 1.0;
	double m_pivotNS = 0.0;
	double m_pivotEW = 0.0;
	double m_pivotVertical = 0.0;
	double m_otaOffset = 0.0;
	double m_latitude = 45.0;
	double m_tubeLength = 0.0;
	double m_apertureDiameter = 0.0;
	SideOfPier m_sideOfPier = SideOfPierAuto;

	// state
	double m_domeAz = 0.0;
	double m_scopeAz = 0.0;
	double m_scopeAlt = 45.0;
	double m_shutterPosition = 0.0;
	bool m_shutterBusy = false;
	bool m_domeBusy = false;
	bool m_blinkOn = false;
	QTimer m_blinkTimer;
	bool m_showTelescope = true;

	// appearance
	QString m_title = "Dome";
	QColor m_backgroundColor = QColor(28, 30, 34);
	QColor m_domeColor = QColor(190, 200, 214);
	QColor m_openingColor = QColor(24, 44, 68);
	QColor m_telescopeColor = QColor(188, 191, 198);
	QColor m_labelColor = QColor(190, 195, 205);
	QColor m_okColor = QColor(80, 200, 120);
	QColor m_blockedColor = QColor(226, 96, 82);
	/* The warm tone of set_busy() in widget_state.h, pale enough to read as
	   a translucent panel fill. */
	QColor m_busyColor = QColor(226, 186, 108);
	qreal m_domeOpacity = 0.35;
	bool m_showLabels = true;
	bool m_showStatus = true;
	bool m_showCompass = true;

	// cached scene mapping, recomputed on every paint
	QPointF m_center;
	double m_scale = 1.0;  // pixels per meter

	/** Runs the 1Hz blink while anything is busy, stops it when nothing is. */
	void updateBlink();
	void updateTransform();
	QPointF toScreen(double east, double north) const;
	QPointF toScreen(const Vec3 &v) const { return toScreen(v.x, v.y); }

	double effectiveTubeLength() const;
	double effectiveApertureDiameter() const;
	double slitHalfWidth() const;
	Vec3 pointingVector() const;
	Vec3 pivotPoint() const;
	Vec3 otaReferencePoint() const;
	/** Intersection of the line of sight with the dome shell. Returns false
	 *  if the optical axis reference point lies outside the dome. */
	bool lineOfSightExit(Vec3 *exit) const;
	bool exitPointInSlit(const Vec3 &exit) const;

	double wallWidth() const;
	/** Widget coordinates from dome coordinates - the dome turns with it. */
	QTransform domeTransform() const;
	/** The slit seen from above, in dome coordinates - it runs along -y. */
	QPainterPath localSlitPath() const;
	/** The slit seen from above, in widget coordinates. */
	QPainterPath slitPath() const;
	/** The part of the slit the leaves have uncovered, in dome and in widget
	 *  coordinates. Empty while the shutter is closed. */
	QPainterPath localOpeningPath() const;
	QPainterPath openingPath() const;
	/** How far the closed shutter reaches sideways past the slit, in pixels.
	 *  The two ends have overlaps of their own. */
	double shutterOverlap() const;
	/** Width of one shutter leaf in pixels - half the slit plus the overlap,
	 *  so the closed pair covers the slit with that margin all round. */
	double shutterLeafWidth() const;
	/** One leaf of the shutter, in dome coordinates, starting at fromX. The
	 *  leaves ride on top of the dome, so nothing clips them. */
	QPainterPath localShutterLeaf(double fromX) const;

	void drawCompass(QPainter &painter);
	void drawOpening(QPainter &painter);
	void drawDomeGuides(QPainter &painter);
	void drawDome(QPainter &painter);
	void drawShutter(QPainter &painter);
	void drawTelescope(QPainter &painter);
	void drawLineOfSight(QPainter &painter);
	void drawExitMarkers(QPainter &painter);
	void drawLabels(QPainter &painter);

	static double normalizeAzimuth(double azimuth);
};
