#include "inspection_overlay.h"
#include "image_inspector.h"
#include "imagepreview.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <cmath>
#include <QtConcurrent/QtConcurrentRun>
#include <QApplication>
#include <QFuture>

InspectionOverlay::InspectionOverlay(QWidget *parent)
	: QWidget(parent), m_center_hfd(0), m_opacity(0.85),
	  m_center_detected(0), m_center_used(0), m_center_rejected(0)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void InspectionOverlay::runInspection(const preview_image &img) {
	// prepare a heap copy of the image (preview_image copy may require non-const ref)
	preview_image *pimg = new preview_image(const_cast<preview_image&>(const_cast<preview_image&>(img)));

	// increment sequence token and cancel previous watcher if any
	const uint64_t seq = ++m_seq;
	if (m_watcher) {
		try { m_watcher->future().cancel(); } catch (...) {}
		m_watcher->deleteLater();
		m_watcher = nullptr;
	}

	// compute base image radius now (we'll need it on the GUI thread)
	double base_image_px = std::min(img.width(), img.height()) * 0.2;

	// show busy cursor
	//if (!QApplication::overrideCursor()) QApplication::setOverrideCursor(Qt::BusyCursor);

	QFuture<InspectionResult> future = QtConcurrent::run([pimg]() {
		ImageInspector inspector;
		InspectionResult r = inspector.inspect(*pimg);
		delete pimg;
		return r;
	});

	m_watcher = new QFutureWatcher<InspectionResult>(this);
	m_watcher->setFuture(future);
	connect(m_watcher, &QFutureWatcher<InspectionResult>::finished, this, [this, seq, base_image_px]() {
		// only apply result if still latest
		if (seq != m_seq.load()) {
			if (m_watcher) {
				m_watcher->deleteLater();
				m_watcher = nullptr;
			}
			return;
		}

		InspectionResult r = m_watcher->future().result();

		// compute scene->view pixel scale
		double pixelScale = 1.0;
		if (m_viewptr) {
			QPointF a = m_viewptr->mapFromScene(QPointF(0,0));
			QPointF b = m_viewptr->mapFromScene(QPointF(1,0));
			pixelScale = std::hypot(b.x() - a.x(), b.y() - a.y());
		}
		setInspectionResult(r.dirs, r.center_hfd, r.detected_dirs, r.used_dirs, r.rejected_dirs,
							r.center_detected, r.center_used, r.center_rejected,
							r.used_points, r.used_radii, r.rejected_points,
				    pixelScale, base_image_px);

		if (m_watcher) {
			m_watcher->deleteLater();
			m_watcher = nullptr;
		}

		// restore cursor (only for the latest run)
		//if (QApplication::overrideCursor()) QApplication::restoreOverrideCursor();
	});
}

void InspectionOverlay::setInspectionResult(const std::vector<double> &directions, double center_hfd) {
	m_dirs = directions;
	m_center_hfd = center_hfd;
	// clear counts if none provided
	m_detected.clear();
	m_used.clear();
	m_rejected.clear();
	m_center_detected = m_center_used = m_center_rejected = 0;
	// default scale
	m_pixel_scale = 1.0;
	m_base_image_px = 0.0;
	update();
}

void InspectionOverlay::setInspectionResult(const std::vector<double> &directions, double center_hfd,
										   const std::vector<int> &detected, const std::vector<int> &used, const std::vector<int> &rejected,
										   int center_detected, int center_used, int center_rejected) {
	m_dirs = directions;
	m_center_hfd = center_hfd;
	m_detected = detected;
	m_used = used;
	m_rejected = rejected;
	m_center_detected = center_detected;
	m_center_used = center_used;
	m_center_rejected = center_rejected;
	// default scale
	m_pixel_scale = 1.0;
	m_base_image_px = 0.0;
	update();
}

void InspectionOverlay::setInspectionResult(const std::vector<double> &directions, double center_hfd,
										   const std::vector<int> &detected, const std::vector<int> &used, const std::vector<int> &rejected,
										   int center_detected, int center_used, int center_rejected,
					   const std::vector<QPointF> &used_points, const std::vector<double> &used_radii, const std::vector<QPointF> &rejected_points,
					   double pixel_scale, double base_image_px) {
	m_dirs = directions;
	m_center_hfd = center_hfd;
	m_detected = detected;
	m_used = used;
	m_rejected = rejected;
	m_center_detected = center_detected;
	m_center_used = center_used;
	m_center_rejected = center_rejected;
	m_used_points = used_points;
	m_used_radii = used_radii;
	m_rejected_points = rejected_points;
	// store view scaling and base image radius for zoom-aware drawing
	m_pixel_scale = (pixel_scale > 0.0) ? pixel_scale : 1.0;
	m_base_image_px = (base_image_px > 0.0) ? base_image_px : 0.0;
	update();
}

void InspectionOverlay::setWidgetOpacity(double opacity) {
	m_opacity = opacity;
	update();
}

void InspectionOverlay::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);
	if (m_dirs.empty()) return;

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(QPen(QColor(255, 0, 255, static_cast<int>(m_opacity * 255)), 2));
	p.setBrush(Qt::NoBrush);

	QRectF imgRect = rect();
	QPointF center = imgRect.center();

	// Base radius: prefer image-space base converted to view coords (zoom-aware)
	// Clamp to at most 25% of the smaller view dimension so octagon diameter ~= 50% of visible area
	double base_view = 0.0;
	double max_base = std::min(imgRect.width(), imgRect.height()) * 0.25;
	if (m_base_image_px > 0.0 && m_pixel_scale > 0.0) {
		base_view = m_base_image_px * m_pixel_scale;
		if (base_view > max_base) base_view = max_base;
	} else {
		base_view = max_base;
	}

	// Use center HFD as the reference for scaling vertices so the reference circle represents center HFD
	double center_ref = (m_center_hfd > 0) ? m_center_hfd : 1.0;

	QPolygonF poly;
	for (int i = 0; i < 8; ++i) {
		// start at North (up) and proceed clockwise in 45deg steps
		double angle = M_PI_2 - M_PI_4 * i; // angles: 90,45,0,-45,...
		double h = m_dirs[i];
		double scale = (h > 0) ? (h / center_ref) : 1.0;
		double r = base_view * scale;
		double x = center.x() + r * std::cos(angle);
		double y = center.y() - r * std::sin(angle); // y inverted for screen coords
		poly << QPointF(x, y);
	}

	// Draw octagon stroke (dashed) with width scaled by pixel scale
	QColor octColor = QColor(180, 220, 255, static_cast<int>(m_opacity * 230));
	double penWidth = std::max(1.0, 1.0 * m_pixel_scale);
	QPen polyPen(octColor, penWidth, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
	p.setPen(polyPen);
	p.setBrush(Qt::NoBrush);
	p.drawPolygon(poly);

	// draw a reference circle corresponding to the center HFD (solid, distinct color).
	// The circle radius equals base_view which represents center HFD in the vertex scaling.
	if (m_center_hfd > 0) {
		double cref = base_view;
		QPen circPen(QColor(255, 200, 60, static_cast<int>(m_opacity * 220)), std::max(1.0, 1.0 * m_pixel_scale), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
		p.setPen(circPen);
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(center, cref, cref);
	}



	// draw center marker
	p.setPen(QPen(QColor(200,200,200,static_cast<int>(m_opacity*255)),1));
	p.drawEllipse(center, 3, 3);

	// precompute center label rectangle so vertex labels can avoid overlapping it
	QRectF centerLabelRect;
	if (m_center_hfd > 0) {
		QFont cfont = p.font();
		int centerFont = std::max(7, static_cast<int>(std::min(width(), height()) * 0.015));
		cfont.setPointSize(centerFont);
		cfont.setBold(true);
		QFontMetrics cfm(cfont);
		QString ctxt = QString("C: %1 px").arg(QString::number(m_center_hfd, 'f', 2));
		QRectF ctb = cfm.boundingRect(ctxt);
		QPointF cdraw(center.x() - ctb.width()/2.0, center.y() - ctb.height()/2.0 - 6);
		centerLabelRect = QRectF(cdraw.x()-4, cdraw.y()-2, ctb.width()+8, ctb.height()+4);
	} else {
		centerLabelRect = QRectF();
	}

	// draw used/rejected star markers BEFORE labels so labels remain on top
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(Qt::NoPen);
	QColor usedColor = QColor(0, 200, 120, static_cast<int>(m_opacity * 230));
	for (size_t i = 0; i < m_used_points.size(); ++i) {
		QPointF scenePt = m_used_points[i];
		QPointF pview = (m_viewptr != nullptr) ? m_viewptr->mapFromScene(scenePt) : scenePt;
		double r_scene = (i < m_used_radii.size() ? m_used_radii[i] : std::max(1.5, 3.0 * m_pixel_scale));
		double r = (m_viewptr != nullptr) ? (r_scene * m_pixel_scale) : r_scene;
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(usedColor, std::max(1.0, 1.0 * m_pixel_scale)));
		p.drawEllipse(pview, r, r);
	}
	for (const QPointF &pt : m_rejected_points) {
		QPointF scenePt = pt;
		QPointF pview = (m_viewptr != nullptr) ? m_viewptr->mapFromScene(scenePt) : scenePt;
		double s = std::max(3.0, 3.0 * m_pixel_scale);
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(QColor(255, 0, 0, static_cast<int>(m_opacity*220)), std::max(1.0, 1.0 * m_pixel_scale)));
		p.drawLine(QPointF(pview.x()-s, pview.y()-s), QPointF(pview.x()+s, pview.y()+s));
		p.drawLine(QPointF(pview.x()-s, pview.y()+s), QPointF(pview.x()+s, pview.y()-s));
	}

	// draw labels for each octagon vertex (average HFD)
	if (m_dirs.size() >= 8) {
		QFont baseFont = p.font();
		int fontSize = std::max(6, static_cast<int>(std::min(width(), height()) * 0.015));
		QFont mainFont = baseFont;
		mainFont.setPointSize(fontSize);
		mainFont.setBold(true);
		int smallSize = std::max(6, static_cast<int>(std::min(width(), height()) * 0.012));
		QFont smallFont = baseFont;
		smallFont.setPointSize(smallSize);
		smallFont.setBold(false);

		double vertex_r_view = std::max(2.0, 3.0 * m_pixel_scale);
		double north_effective_r = std::max(4.0, 5.0 * m_pixel_scale);

		struct LabelInfo { QPointF drawPos; double boxW, boxH; QString mainTxt; QString cntTxt; QFont mainF; QFont smallF; };
		std::vector<LabelInfo> labels;

		for (int i = 0; i < 8; ++i) {
			QPointF pt = poly.at(i);
			QPointF dir = pt - center;
			double len = std::hypot(dir.x(), dir.y());

			QString mainTxt = QString::number(m_dirs[i], 'f', 2) + " px";
			QString cntTxt;
			if (!m_detected.empty() && i < static_cast<int>(m_detected.size())) {
				cntTxt = QString("D:%1 U:%2 R:%3").arg(m_detected[i]).arg(m_used[i]).arg(m_rejected[i]);
			}

			QFontMetrics fmMain(mainFont);
			QFontMetrics fmSmall(smallFont);
			QRectF tbMain = fmMain.boundingRect(mainTxt);
			QRectF tbSmall = fmSmall.boundingRect(cntTxt);
			double padX = 8.0;
			double padY = 4.0;
			double boxW = std::max(tbMain.width(), tbSmall.width()) + padX;
			double boxH = tbMain.height() + (cntTxt.isEmpty() ? 0.0 : tbSmall.height()) + padY;

			double base_effective = (i == 0) ? north_effective_r : vertex_r_view;
			double gap = std::max(4.0, 3.0 * m_pixel_scale);

			QPointF drawPos;
			if (len > 1e-6) {
				QPointF unit(dir.x() / len, dir.y() / len);
				double proj = std::abs(unit.x()) * (boxW * 0.5) + std::abs(unit.y()) * (boxH * 0.5);
				double desiredCenterDist = base_effective + gap + proj;
				QPointF labelCenter = QPointF(pt.x() + unit.x() * desiredCenterDist, pt.y() + unit.y() * desiredCenterDist);
				drawPos = QPointF(labelCenter.x() - boxW * 0.5, labelCenter.y() - boxH * 0.5);
				QRectF labelRect(drawPos.x()-4, drawPos.y()-2, boxW, boxH);
				if (!centerLabelRect.isNull() && labelRect.intersects(centerLabelRect)) {
					double extra = (std::max(boxW, boxH) * 0.5) + gap;
					labelCenter = QPointF(pt.x() + unit.x() * (desiredCenterDist + extra), pt.y() + unit.y() * (desiredCenterDist + extra));
					drawPos = QPointF(labelCenter.x() - boxW * 0.5, labelCenter.y() - boxH * 0.5);
				}
			} else {
				drawPos = QPointF(pt.x() - boxW * 0.5, pt.y() - boxH * 0.5);
			}

			labels.push_back({drawPos, boxW, boxH, mainTxt, cntTxt, mainFont, smallFont});
		}

		// draw small vertex markers first
		for (int i = 0; i < poly.size(); ++i) {
			QPointF v = poly.at(i);
			double vr = std::max(2.0, 3.0 * m_pixel_scale);
			QPen vp(Qt::black, std::max(1.5, 1.2 * m_pixel_scale));
			vp.setJoinStyle(Qt::RoundJoin);
			p.setPen(vp);
			p.setBrush(octColor);
			p.drawEllipse(v, vr, vr);
		}

		// draw labels on top of vertices
		for (size_t i = 0; i < labels.size(); ++i) {
			const LabelInfo &li = labels[i];
			QColor bg = QColor(0,0,0, static_cast<int>(m_opacity*180));
			p.setBrush(bg);
			p.setPen(Qt::NoPen);
			QRectF bgRect(li.drawPos.x()-4, li.drawPos.y()-2, li.boxW, li.boxH);
			p.drawRoundedRect(bgRect, 3, 3);

			// draw main text
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)),1));
			p.setFont(li.mainF);
			QFontMetrics fmMain2(li.mainF);
			QRectF tbMain2 = fmMain2.boundingRect(li.mainTxt);
			double mx = li.drawPos.x() + (li.boxW - tbMain2.width()) * 0.5;
			double my = li.drawPos.y() + tbMain2.height();
			p.drawText(mx, my, li.mainTxt);

			// draw counts if present
			if (!li.cntTxt.isEmpty()) {
				p.setFont(li.smallF);
				QFontMetrics fmSmall2(li.smallF);
				QRectF tbSmall2 = fmSmall2.boundingRect(li.cntTxt);
				double sx = li.drawPos.x() + (li.boxW - tbSmall2.width()) * 0.5;
				double sy = li.drawPos.y() + fmMain2.height() + tbSmall2.height();
				p.setPen(QPen(QColor(200,200,200,static_cast<int>(m_opacity*255)),1));
				p.drawText(sx, sy, li.cntTxt);
			}
		}
	}

	// draw center HFD label (smaller and nudge down if it would overlap the north vertex)
	if (m_center_hfd > 0) {
		QFont font = p.font();
		int centerFont = std::max(7, static_cast<int>(std::min(width(), height()) * 0.015));
		font.setPointSize(centerFont);
		font.setBold(true);
		p.setFont(font);
		QString txt = QString("C: %1 px").arg(QString::number(m_center_hfd, 'f', 2));
		QRectF tb = p.fontMetrics().boundingRect(txt);
		// prefer centered position but nudge down if it intersects the north vertex
		QPointF labelPos(center.x(), center.y());
		QPointF drawPos(labelPos.x() - tb.width()/2.0, labelPos.y() - tb.height()/2.0 - 6);

		// if north vertex exists and overlaps the center label, push the label down
		if (!poly.isEmpty()) {
			QPointF north = poly.at(0);
			double north_r = std::max(3.0, 4.0 * m_pixel_scale);
			QRectF northRect(north.x() - (north_r + 4.0), north.y() - (north_r + 4.0), (north_r + 4.0) * 2.0, (north_r + 4.0) * 2.0);
			QRectF centerBg(drawPos.x()-4, drawPos.y()-2, tb.width()+8, tb.height()+4);
			if (centerBg.intersects(northRect)) {
				double push = (north_r + 6.0);
				drawPos.setY(drawPos.y() + push);
			}
		}

		QColor bg = QColor(0,0,0, static_cast<int>(m_opacity*180));
		p.setBrush(bg);
		p.setPen(Qt::NoPen);
		QRectF bgRect(drawPos.x()-4, drawPos.y()-2, tb.width()+8, tb.height()+4);
		p.drawRoundedRect(bgRect, 3, 3);
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)),1));
		p.drawText(drawPos.x(), drawPos.y() + tb.height(), txt);

		// draw center counts if available (small)
		if (m_center_detected > 0 || m_center_used > 0 || m_center_rejected > 0) {
			QFont small = p.font();
			small.setPointSize(std::max(6, static_cast<int>(std::min(width(), height()) * 0.012)));
			small.setBold(false);
			p.setFont(small);
			QString cnt = QString("D:%1 U:%2 R:%3").arg(m_center_detected).arg(m_center_used).arg(m_center_rejected);
			QRectF tb2 = p.fontMetrics().boundingRect(cnt);
			QPointF drawPos2(labelPos.x() - tb2.width()/2.0, drawPos.y() + tb.height() + 2);
			QColor bg2 = QColor(0,0,0, static_cast<int>(m_opacity*160));
			p.setBrush(bg2);
			p.setPen(Qt::NoPen);
			QRectF bgRect2(drawPos2.x()-3, drawPos2.y()-2, tb2.width()+6, tb2.height()+4);
			p.drawRoundedRect(bgRect2, 3, 3);
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(QColor(200,200,200,static_cast<int>(m_opacity*255)),1));
			p.drawText(drawPos2.x(), drawPos2.y() + tb2.height(), cnt);
		}
	}

	// vertices and labels drawn above in the block above


}
