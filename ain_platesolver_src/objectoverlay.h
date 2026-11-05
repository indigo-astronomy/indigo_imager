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


#ifndef OBJECTOVERLAY_H
#define OBJECTOVERLAY_H

#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QColor>
#include <QPointF>
#include <QString>
#include <vector>
#include <imagepreview.h>

// Deep sky objects and stars of indigocat drawn over a solved image. The items are children of the
// image item of the viewer, so they zoom and pan with it and the viewer owns them. They are clipped to the image area.
class ObjectOverlay {
public:
	explicit ObjectOverlay(QGraphicsItem *image_item);

	// Draws the selected kinds of objects and returns how many of each are in the field. RA and Dec of the image are JNow if jnow is set, otherwise J2000
	void show(const preview_image &image, bool jnow, bool show_deep_sky, bool show_stars, int *deep_sky_count, int *star_count);
	void clear();

	// Places the labels so that they do not overlap at the current zoom, call it when the zoom changes
	void layout_labels();

private:
	struct object_label {
		QGraphicsSimpleTextItem *text;
		QPointF anchor;
		double radius;      // image pixels
		double rank;
	};

	void add_marker(double x, double y, double rank, const QColor &color, const QString &label, const QString &tooltip);
	// false if nothing of the ellipse is in the image, then it is not added
	bool add_ellipse(double x, double y, double major, double minor, double rotation, double rank, const QColor &color, const QString &label, const QString &tooltip);
	void add_label(double x, double y, double radius, double rank, const QColor &color, const QString &label);

	QGraphicsRectItem *m_root;
	std::vector<object_label> m_labels;
	std::vector<QPointF> m_markers;
	double m_marker_radius;
};

#endif // OBJECTOVERLAY_H
