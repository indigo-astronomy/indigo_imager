#include "image_inspector_overlay.h"
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
#include <chrono>
#include <cstdio>

ImageInspectorOverlay::ImageInspectorOverlay(QWidget *parent)
	: QWidget(parent), m_center_hfd(0), m_opacity(0.85),
	  m_center_detected(0), m_center_used(0), m_center_rejected(0)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void ImageInspectorOverlay::setView(QGraphicsView *view) {
	// Remove previous filter/parenting if any
	if (m_viewptr) {
		if (m_viewptr->viewport()) m_viewptr->viewport()->removeEventFilter(this);
		// don't delete parent; just detach
		this->setParent(nullptr);
	}

	m_viewptr = view;
	if (!m_viewptr) return;

	QWidget *vp = m_viewptr->viewport();
	if (vp) {
		// make the overlay a child of the viewport so it moves/resizes with it
		this->setParent(vp);
		this->setAttribute(Qt::WA_TransparentForMouseEvents);
		this->setAttribute(Qt::WA_NoSystemBackground, true);
		this->setGeometry(0, 0, vp->width(), vp->height());
		this->show();
		vp->installEventFilter(this);
	}
}

bool ImageInspectorOverlay::eventFilter(QObject *watched, QEvent *event) {
	if (m_viewptr && m_viewptr->viewport() && watched == m_viewptr->viewport()) {
		if (event->type() == QEvent::Resize) {
			// Ensure overlay matches viewport size and recompute UI scale
			QWidget *vp = m_viewptr->viewport();
			if (vp) {
				this->setGeometry(0, 0, vp->width(), vp->height());
				// recompute pixel scale via helper
				m_pixel_scale = computeUiPixelScale();
				update();
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}

// helper: compute a UI-only pixel scale based on the overlay/widget short dimension
double ImageInspectorOverlay::computeUiPixelScale() const {
	const double REF_VIEW_SHORT = 800.0;
	int vw = this->width();
	int vh = this->height();
	if (vw > 0 && vh > 0) {
		double shortDim = static_cast<double>(std::min(vw, vh));
		return std::max(0.20, std::min(shortDim / REF_VIEW_SHORT, 4.0));
	}
	return 1.0;
}

void ImageInspectorOverlay::runInspection(const preview_image &img) {
	// prepare a heap copy of the image (preview_image copy may require non-const ref)
	preview_image *pimg = new preview_image(const_cast<preview_image&>(const_cast<preview_image&>(img)));

	// increment sequence token and cancel previous watcher if any
	const uint64_t seq = ++m_seq;
	if (m_watcher) {
		try { m_watcher->future().cancel(); } catch (...) {}
		m_watcher->deleteLater();
		m_watcher = nullptr;
	}

	// clear any existing inspection overlay immediately so UI doesn't show stale results
	setInspectionResult(InspectionResult());

	// set busy message so paintEvent can show a visual hint while analysis runs
	m_busy_message = "Analyzing image...";
	update();

	// compute base image radius now (we'll need it on the GUI thread)
	double base_image_px = std::min(img.width(), img.height()) * 0.2;

	// show busy cursor
	// if (!QApplication::overrideCursor()) QApplication::setOverrideCursor(Qt::BusyCursor);

	QFuture<InspectionResult> future = QtConcurrent::run([pimg]() {
		auto t0 = std::chrono::high_resolution_clock::now();
		ImageInspector inspector;
		InspectionResult r = inspector.inspect(*pimg);
		auto t1 = std::chrono::high_resolution_clock::now();
		double elapsed_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();
		std::fprintf(stderr, "ImageInspector::inspect (overlay): %.3f ms\n", elapsed_ms);
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

		m_busy_message.clear();

		double pixelScale = computeUiPixelScale();
		setInspectionResult(r);

		m_pixel_scale = (pixelScale > 0.0) ? pixelScale : 1.0;
		m_base_image_px = (base_image_px > 0.0) ? base_image_px : 0.0;
		update();

		if (m_watcher) {
			m_watcher->deleteLater();
			m_watcher = nullptr;
		}

		// restore cursor (only for the latest run)
		// if (QApplication::overrideCursor()) QApplication::restoreOverrideCursor();
	});
}

void ImageInspectorOverlay::clearInspection() {
	// bump sequence token to invalidate any pending result
	++m_seq;
	if (m_watcher) {
		try { m_watcher->future().cancel(); } catch (...) {}
		m_watcher->deleteLater();
		m_watcher = nullptr;
	}
	// clear displayed results
	setInspectionResult(InspectionResult());
}

void ImageInspectorOverlay::setInspectionResult(const InspectionResult &res) {
	m_dirs = res.dirs;
	m_center_hfd = res.center_hfd;
	m_detected = res.detected_dirs;
	m_used = res.used_dirs;
	m_rejected = res.rejected_dirs;
	m_center_detected = res.center_detected;
	m_center_used = res.center_used;
	m_center_rejected = res.center_rejected;
	m_used_points = res.used_points;
	m_used_radii = res.used_radii;
	m_rejected_points = res.rejected_points;
	if (!res.cell_eccentricity.empty()) {
		m_cell_eccentricity = res.cell_eccentricity;
	} else {
		m_cell_eccentricity.clear();
	}
	if (!res.cell_major_angle.empty()) {
		m_cell_major_angle = res.cell_major_angle;
	} else {
		m_cell_major_angle.clear();
	}
	m_error_message = res.error_message;
	// Note: view-specific scaling (m_pixel_scale/m_base_image_px) is set by the caller
	update();
}

void ImageInspectorOverlay::setWidgetOpacity(double opacity) {
	m_opacity = opacity;
	update();
}

// Draw a subtle busy indicator (octagon) behind messages
void ImageInspectorOverlay::drawBusyIndicator(QPainter &p, const QRectF &r) {
	QPointF center = r.center();
	double base = std::min(r.width(), r.height()) * 0.15;
	if (base < 8.0) {
		base = 8.0;
	}

	QPolygonF poly;
	for (int i = 0; i < 8; ++i) {
		double angle = M_PI_2 - M_PI_4 * i;
		double x = center.x() + base * std::cos(angle);
		double y = center.y() - base * std::sin(angle);
		poly << QPointF(x, y);
	}

	QColor fillCol = QColor(50, 80, 120, static_cast<int>(m_opacity * 60));
	QColor edgeCol = QColor(180, 220, 255, static_cast<int>(m_opacity * 120));
	p.save();
	p.setBrush(fillCol);
	p.setPen(Qt::NoPen);
	p.drawPolygon(poly);

	QPen pen(edgeCol, std::max(1.0, 1.0 * m_pixel_scale), Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	p.drawPolygon(poly);
	p.restore();
}

// Draw a centered header + message (error or busy) and optional busy indicator
void ImageInspectorOverlay::drawCenterMessage(QPainter &p) {
	QFont headerFont = p.font();
	int headerFs = std::max(12, static_cast<int>(std::min(width(), height()) * 0.03));
	headerFont.setPointSize(headerFs);
	headerFont.setBold(true);

	QFont msgFont = p.font();
	int msgFs = std::max(8, static_cast<int>(headerFs * 0.8));
	msgFont.setPointSize(msgFs);
	msgFont.setBold(false);

	QString header = tr("Image inspector");
	QString msg = !m_error_message.empty() ? QString::fromUtf8(m_error_message.c_str()) : QString::fromUtf8(m_busy_message.c_str());

	// Measure sizes and draw both lines centered as a block
	QFontMetrics hf(headerFont);
	QFontMetrics mf(msgFont);
	QRectF r = rect();

	// draw busy indicator behind the text so message remains readable
	if (!m_busy_message.empty()) drawBusyIndicator(p, r);

	QRectF hb = hf.boundingRect(r.toRect(), Qt::AlignCenter | Qt::TextWordWrap, header);
	QRectF mb = mf.boundingRect(r.toRect(), Qt::AlignCenter | Qt::TextWordWrap, msg);

	double blockH = hb.height() + mb.height();
	double startY = r.top() + (r.height() - blockH) / 2.0;

	double shadowOffset = (m_pixel_scale > 0.0) ? std::max(1.0, 1.5 * m_pixel_scale) : 1.0;
	QColor shadowCol = QColor(0, 0, 0, static_cast<int>(m_opacity * 220));

	p.setFont(headerFont);
	QRectF headerRect(r.left(), startY, r.width(), hb.height());
	QRectF headerRectShadow = headerRect.translated(shadowOffset, shadowOffset);
	p.setPen(QPen(shadowCol));
	p.drawText(headerRectShadow, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, header);
	p.setPen(QPen(QColor(255,255,255,255)));
	p.drawText(headerRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, header);

	p.setFont(msgFont);
	QRectF msgRect(r.left(), startY + hb.height(), r.width(), mb.height());
	QRectF msgRectShadow = msgRect.translated(shadowOffset, shadowOffset);
	p.setPen(QPen(shadowCol));
	p.drawText(msgRectShadow, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, msg);
	p.setPen(QPen(QColor(220,220,220,255)));
	p.drawText(msgRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, msg);
}

void ImageInspectorOverlay::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);

	const QString hfdFormat = QStringLiteral("%1 px");
	const QString morphFormat = QStringLiteral("ε:%1 ∠%2°");
	const QString countFormat = QStringLiteral("★%1 ✓%2 ✕%3");
	const double padX = 8.0;
	const double padY = 4.0;

	QPainter p(this);
	QFont hfdFont = p.font();
	int hfdFontSize = std::max(12, static_cast<int>(std::min(width(), height()) * 0.015));
	hfdFont.setPointSize(hfdFontSize);
	hfdFont.setBold(true);
	QFontMetrics fmHfd(hfdFont);

	QFont morphFont = p.font();
	int morphFontSize = std::max(10, static_cast<int>(std::min(width(), height()) * 0.012));
	morphFont.setPointSize(morphFontSize);
	morphFont.setBold(false);
	QFontMetrics fmMorph(morphFont);

	if (!m_error_message.empty() || !m_busy_message.empty()) {
		p.setRenderHint(QPainter::Antialiasing, true);
		drawCenterMessage(p);
		return;
	}

	if (m_dirs.empty()) return;

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

	// precompute center label rectangle so vertex labels can avoid overlapping it
	QRectF centerLabelRect;
	if (m_center_hfd > 0) {
		QString ctxt = morphFormat.arg(QString::number(m_center_hfd, 'f', 2));
		QRectF ctb = fmHfd.boundingRect(ctxt);
		QPointF cdraw(center.x() - ctb.width()/2.0, center.y() - ctb.height()/2.0 - 6);
		centerLabelRect = QRectF(cdraw.x()-4, cdraw.y()-2, ctb.width()+8, ctb.height()+4);
	} else {
		centerLabelRect = QRectF();
	}

	// draw used star markers as oriented ellipses representing morphology/size
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(Qt::NoPen);
	// prepare scene rect and grid size (inspector uses 5x5 by default)
	int gx = 5, gy = 5;
	QRectF sceneRect;
	if (m_viewptr && m_viewptr->scene()) sceneRect = m_viewptr->scene()->sceneRect();

	for (size_t i = 0; i < m_used_points.size(); ++i) {
		QPointF scenePt = m_used_points[i];
		QPointF pview = (m_viewptr != nullptr) ? m_viewptr->mapFromScene(scenePt) : scenePt;

		// compute view-space major radius by mapping a scene-space offset
		double major_view = 6.0 * m_pixel_scale; // fallback
		if (i < m_used_radii.size()) {
			double r_img = m_used_radii[i];
			QPointF pv_edge = (m_viewptr != nullptr) ? m_viewptr->mapFromScene(QPointF(scenePt.x() + r_img, scenePt.y())) : QPointF(pview.x() + r_img, pview.y());
			major_view = std::abs(pv_edge.x() - pview.x());
			if (major_view < 1.0) major_view = 1.0;
		}
		// make ellipse 1.2x actual star size (view-space)
		major_view = std::max(2.0, major_view * 1.2);

		// determine corresponding grid cell to pull morphology (ecc, angle)
		double ecc = 0.0;
		double ang = 0.0;
		if (!sceneRect.isNull() && sceneRect.width() > 0 && sceneRect.height() > 0 && !m_cell_eccentricity.empty()) {
			double cw = sceneRect.width() / gx;
			double ch = sceneRect.height() / gy;
			int cx = std::min(std::max(0, static_cast<int>(scenePt.x() / cw)), gx - 1);
			int cy = std::min(std::max(0, static_cast<int>(scenePt.y() / ch)), gy - 1);
			int idx = cy * gx + cx;
			if (idx >= 0 && idx < static_cast<int>(m_cell_eccentricity.size())) ecc = m_cell_eccentricity[idx];
			if (idx >= 0 && idx < static_cast<int>(m_cell_major_angle.size())) ang = m_cell_major_angle[idx];
		}

		double minor_view = major_view;
		if (!std::isnan(ecc) && ecc > 0.0 && ecc < 1.0) minor_view = major_view * std::sqrt(std::max(0.0, 1.0 - ecc * ecc));

		// Use semi-trasparent green for used stars
		QColor edgeCol = QColor(0,200,120, static_cast<int>(m_opacity*160));
		QColor fillCol = QColor(0,200,120, static_cast<int>(m_opacity*40));

		p.save();
		p.translate(pview);
		p.rotate(-ang + 90.0);
		p.setPen(QPen(edgeCol, std::max(1.0, 1.0 * m_pixel_scale)));
		p.setBrush(QBrush(fillCol));
		p.drawEllipse(QPointF(0,0), major_view, minor_view);
		p.restore();
	}

	// draw labels for each octagon vertex (average HFD)
	if (m_dirs.size() >= 8) {
		double vertex_r_view = std::max(3.0, 4.0 * m_pixel_scale);
		double north_effective_r = std::max(8.0, 10.0 * m_pixel_scale);

		struct LabelInfo { QPointF drawPos; double boxW, boxH; QString mainTxt; QString morphTxt; QString cntTxt; QColor edgeCol; QColor lineCol; };
		std::vector<LabelInfo> labels;

		// precompute per-vertex effective radii to avoid label overlap with enlarged ellipses
		std::vector<double> vertexEffective(8, 0.0);
		for (int i = 0; i < 8; ++i) {
			QPointF pt = poly.at(i);
			QPointF dir = pt - center;
			double len = std::hypot(dir.x(), dir.y());
			// compute representative major axis (same logic as ellipse drawing)
			double maj = (m_dirs.size() > static_cast<size_t>(i) && m_dirs[i] > 0) ? (m_dirs[i] * m_pixel_scale) : (base_view * 0.20);
			maj = std::min(maj, base_view * 0.6);
			maj = std::max(2.0, maj);
			maj *= 4.0;
			double base_effective = (i == 0) ? north_effective_r : vertex_r_view;
			// offset label center by half the major axis length plus a larger gap
			vertexEffective[i] = base_effective + (maj * 0.5) + std::max(12.0, 6.0 * m_pixel_scale);

			QString mainTxt = hfdFormat.arg(QString::number(m_dirs[i], 'f', 2));
			QString morphTxt;
			QString cntTxt;
				if (!m_detected.empty() && i < static_cast<int>(m_detected.size())) {
					cntTxt = countFormat.arg(m_detected[i]).arg(m_used[i]).arg(m_rejected[i]);
				}
			// map vertex index -> grid cell to fetch eccentricity/angle
			int vx_cx = 0, vx_cy = 0;
			switch (i) {
				case 0: vx_cx = 2; vx_cy = 0; break; // N
				case 1: vx_cx = 4; vx_cy = 0; break; // NE
				case 2: vx_cx = 4; vx_cy = 2; break; // E
				case 3: vx_cx = 4; vx_cy = 4; break; // SE
				case 4: vx_cx = 2; vx_cy = 4; break; // S
				case 5: vx_cx = 0; vx_cy = 4; break; // SW
				case 6: vx_cx = 0; vx_cy = 2; break; // W
				case 7: vx_cx = 0; vx_cy = 0; break; // NW
			}
			int v_idx = vx_cy * 5 + vx_cx;
			double vecc = (v_idx >= 0 && v_idx < static_cast<int>(m_cell_eccentricity.size())) ? m_cell_eccentricity[v_idx] : 0.0;
			double vang = (v_idx >= 0 && v_idx < static_cast<int>(m_cell_major_angle.size())) ? m_cell_major_angle[v_idx] : 0.0;
			if (!std::isnan(vecc) && vecc > 0.0) {
				morphTxt = morphFormat.arg(QString::number(vecc, 'f', 2)).arg(QString::number(vang, 'f', 0));
			}

			QRectF tbMain = fmHfd.boundingRect(mainTxt);
			QRectF tbMorph = fmMorph.boundingRect(morphTxt);
			QRectF tbSmall = fmMorph.boundingRect(cntTxt);
			double boxW = std::max({tbMain.width(), tbMorph.width(), tbSmall.width()}) + 2 * padX;
			double boxH = tbMain.height() + (morphTxt.isEmpty() ? 0.0 : tbMorph.height()) + (cntTxt.isEmpty() ? 0.0 : tbSmall.height()) + padY;

			double gap = std::max(12.0, 6.0 * m_pixel_scale);

			QPointF drawPos;
			if (len > 1e-6) {
				QPointF unit(dir.x() / len, dir.y() / len);
				double proj = std::abs(unit.x()) * (boxW * 0.5) + std::abs(unit.y()) * (boxH * 0.5);
				double desiredCenterDist = vertexEffective[i] + proj;
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

			// choose label colors to match ellipse coloring (green <0.4, default 0.4-0.55, red >0.55)
			QColor labEdge, labLine;
			if (vecc < 0.4) {
				labEdge = QColor(0,200,120, static_cast<int>(m_opacity*220));
				labLine = QColor(120,240,200, static_cast<int>(m_opacity*220));
			} else if (vecc > 0.55) {
				labEdge = QColor(220,60,60, static_cast<int>(m_opacity*220));
				labLine = QColor(255,140,140, static_cast<int>(m_opacity*220));
			} else {
				labEdge = QColor(255,180,80, static_cast<int>(m_opacity*220));
				labLine = QColor(255,220,120, static_cast<int>(m_opacity*220));
			}
			labels.push_back({drawPos, boxW, boxH, mainTxt, morphTxt, cntTxt, labEdge, labLine});
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

		// draw oriented ellipses at vertices to indicate per-cell eccentricity/angle
		// assume the inspection grid is 5x5 and per-cell vectors follow that layout
		// Always draw the ellipse (do not hide when eccentricity is small)
		int gx = 5;
		for (int i = 0; i < 8; ++i) {
			QPointF v = poly.at(i);
			int cx = 0, cy = 0;
			switch (i) {
				case 0: cx = 2; cy = 0; break; // N
				case 1: cx = 4; cy = 0; break; // NE
				case 2: cx = 4; cy = 2; break; // E
				case 3: cx = 4; cy = 4; break; // SE
				case 4: cx = 2; cy = 4; break; // S
				case 5: cx = 0; cy = 4; break; // SW
				case 6: cx = 0; cy = 2; break; // W
				case 7: cx = 0; cy = 0; break; // NW
			}
			int idx = cy * gx + cx;
			double ecc = (idx >= 0 && idx < static_cast<int>(m_cell_eccentricity.size())) ? m_cell_eccentricity[idx] : 0.0;
			double ang = (idx >= 0 && idx < static_cast<int>(m_cell_major_angle.size())) ? m_cell_major_angle[idx] : 0.0;
			// representative major axis length in view pixels: use vertex HFD if available
			double major_view = (m_dirs.size() > static_cast<size_t>(i) && m_dirs[i] > 0) ? (m_dirs[i] * m_pixel_scale) : (base_view * 0.20);
			major_view = std::min(major_view, base_view * 0.6);
			major_view = std::max(2.0, major_view);
			// make ellipse visually larger (user request): scale up by 4x
			major_view *= 4.0;
			double minor_view = major_view;
			if (ecc > 0.0 && ecc < 1.0) minor_view = major_view * std::sqrt(std::max(0.0, 1.0 - ecc * ecc));
			// always draw a colored ellipse for the cell; keep color thresholds:
			// green (<0.4), default (0.4-0.55), red (>0.55)
			QColor edgeCol, fillCol, lineCol;
			if (ecc < 0.4) {
				edgeCol = QColor(0,200,120, static_cast<int>(m_opacity*220));
				fillCol = QColor(0,200,120, static_cast<int>(m_opacity*80));
				lineCol = QColor(120,240,200, static_cast<int>(m_opacity*220));
			} else if (ecc > 0.55) {
				edgeCol = QColor(220,60,60, static_cast<int>(m_opacity*220));
				fillCol = QColor(220,60,60, static_cast<int>(m_opacity*80));
				lineCol = QColor(255,140,140, static_cast<int>(m_opacity*220));
			} else {
				edgeCol = QColor(255,180,80, static_cast<int>(m_opacity*220));
				fillCol = QColor(255,180,80, static_cast<int>(m_opacity*80));
				lineCol = QColor(255,220,120, static_cast<int>(m_opacity*220));
			}
			p.save();
			p.translate(v);
			p.rotate(-ang + 90.0); // rotate so major axis aligns with computed angle (adjusted +90°)
			QPen ep(edgeCol, std::max(1.0, 1.0 * m_pixel_scale));
			p.setPen(ep);
			QBrush eb(fillCol);
			p.setBrush(eb);
			p.drawEllipse(QPointF(0,0), major_view, minor_view);
			// draw major-axis line (along ellipse major axis)
			p.setPen(QPen(lineCol, std::max(1.0, 1.0 * m_pixel_scale)));
			p.drawLine(QPointF(-major_view, 0), QPointF(major_view, 0));
			p.restore();
		}

		// draw labels on top of vertices
		for (size_t i = 0; i < labels.size(); ++i) {
			const LabelInfo &li = labels[i];
			QColor bg = QColor(0,0,0, static_cast<int>(m_opacity*180));
			p.setBrush(bg);
			p.setPen(Qt::NoPen);
			QRectF bgRect(li.drawPos.x(), li.drawPos.y(), li.boxW, li.boxH);
			p.drawRoundedRect(bgRect, 3, 3);

			// draw main text centered vertically and horizontally within the label box
			p.setBrush(Qt::NoBrush);
			// keep HFD main text white; morphology (ε/∠) uses color-coding
			p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)),1));
			p.setFont(hfdFont);
			QRectF tbHfd = fmHfd.boundingRect(li.mainTxt);
			double mainH = fmHfd.height();
			double morphH = li.morphTxt.isEmpty() ? 0.0 : fmMorph.height();
			double smallH = li.cntTxt.isEmpty() ? 0.0 : fmMorph.height();
			double contentH = mainH + morphH + smallH;
			double topPad = (li.boxH - contentH) * 0.5;
			double mx = li.drawPos.x() + (li.boxW - tbHfd.width()) * 0.5;
			double my = li.drawPos.y() + topPad + fmHfd.ascent();
			p.drawText(mx, my, li.mainTxt);

			// draw morphology (ε / angle) on the second line if present
			if (!li.morphTxt.isEmpty()) {
				p.setFont(morphFont);
				QRectF tbMorph = fmMorph.boundingRect(li.morphTxt);
				double sxm = li.drawPos.x() + (li.boxW - tbMorph.width()) * 0.5;
				double sym = li.drawPos.y() + topPad + mainH + fmMorph.ascent();
				p.setPen(QPen(li.lineCol,1));
				p.drawText(sxm, sym, li.morphTxt);
			}

			// draw counts on the third line if present
			if (!li.cntTxt.isEmpty()) {
				p.setFont(morphFont);
				QRectF tbMorph = fmMorph.boundingRect(li.cntTxt);
				double sx = li.drawPos.x() + (li.boxW - tbMorph.width()) * 0.5;
				double sy = li.drawPos.y() + topPad + mainH + morphH + fmMorph.ascent();
				QColor cntCol = QColor(200,200,200,static_cast<int>(m_opacity*255));
				p.setPen(QPen(cntCol,1));
				p.drawText(sx, sy, li.cntTxt);
			}
		}
	}

	// draw center HFD label (smaller and nudge down if it would overlap the north vertex)
	if (m_center_hfd > 0) {
		p.setFont(hfdFont);
		QString txt = hfdFormat.arg(QString::number(m_center_hfd, 'f', 2));
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

		// compute effective center point from possibly nudged drawPos so combined box aligns
		QPointF effectiveCenter(drawPos.x() + tb.width()/2.0, drawPos.y() + tb.height()/2.0 + 6);

		// Combine main center label and counts into a single background box
		// central morphology (center cell at grid 2,2 -> index 12)
		QString cmorph;
		QRectF tbMorph;
		int center_idx = 2 + 2 * 5;
		double c_ecc = (center_idx >= 0 && center_idx < static_cast<int>(m_cell_eccentricity.size())) ? m_cell_eccentricity[center_idx] : 0.0;
		double c_ang = (center_idx >= 0 && center_idx < static_cast<int>(m_cell_major_angle.size())) ? m_cell_major_angle[center_idx] : 0.0;
		QColor cmorphCol = QColor(255,220,120, static_cast<int>(m_opacity*220));
		if (!std::isnan(c_ecc) && c_ecc > 0.0) {
			cmorph = QString::fromUtf8("ε:%1 ∠%2°").arg(QString::number(c_ecc, 'f', 2)).arg(QString::number(c_ang, 'f', 0));

			if (c_ecc < 0.4) {
				cmorphCol = QColor(120,240,200, static_cast<int>(m_opacity*220));
			} else if (c_ecc > 0.55) {
				cmorphCol = QColor(255,140,140, static_cast<int>(m_opacity*220));
			} else {
				cmorphCol = QColor(255,220,120, static_cast<int>(m_opacity*220));
			}

			tbMorph = fmMorph.boundingRect(cmorph);
		}

		QString cnt;
		QRectF tb2;
		if (m_center_detected > 0 || m_center_used > 0 || m_center_rejected > 0) {
			cnt = countFormat.arg(m_center_detected).arg(m_center_used).arg(m_center_rejected);
			tb2 = fmMorph.boundingRect(cnt);
		}

		double combinedW = std::max({tb.width(), tb2.width(), tbMorph.width()}) + 2 * padX;
		double combinedH = tb.height() + (cmorph.isEmpty() ? 0.0 : tbMorph.height()) + (cnt.isEmpty() ? 0.0 : tb2.height()) + padY;
		QPointF combinedTopLeft(effectiveCenter.x() - combinedW/2.0, effectiveCenter.y() - combinedH/2.0);

		QColor bg = QColor(0,0,0, static_cast<int>(m_opacity*180));
		p.setBrush(bg);
		p.setPen(Qt::NoPen);
		QRectF bgRect(combinedTopLeft.x(), combinedTopLeft.y(), combinedW, combinedH);
		p.drawRoundedRect(bgRect, 3, 3);

		// draw main text, central morphology (colored) and counts centered vertically and horizontally inside the combined box
		p.setBrush(Qt::NoBrush);
		p.setFont(hfdFont);
		QRectF tbMain = fmHfd.boundingRect(txt);
		double mainH = fmHfd.height();
		double morphH = cmorph.isEmpty() ? 0.0 : fmMorph.height();
		double smallH = cnt.isEmpty() ? 0.0 : fmMorph.height();
		double contentH = mainH + morphH + (cnt.isEmpty() ? 0.0 : smallH);
		double topPad = (combinedH - contentH) * 0.5;
		double mainX = combinedTopLeft.x() + (combinedW - tbMain.width()) * 0.5;
		double mainY = combinedTopLeft.y() + topPad + fmHfd.ascent();
		p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)),1));
		p.drawText(mainX, mainY, txt);

		// draw central morphology (colored) below main text
		if (!cmorph.isEmpty()) {
			p.setFont(morphFont);
			double mx = combinedTopLeft.x() + (combinedW - tbMorph.width()) * 0.5;
			double my = combinedTopLeft.y() + topPad + mainH + fmMorph.ascent();
			p.setPen(QPen(cmorphCol,1));
			p.drawText(mx, my, cmorph);
		}

		// draw counts below morphology
		if (!cnt.isEmpty()) {
			p.setFont(morphFont);
			double sx = combinedTopLeft.x() + (combinedW - fmMorph.boundingRect(cnt).width()) * 0.5;
			double sy = combinedTopLeft.y() + topPad + mainH + morphH + fmMorph.ascent();
			p.setPen(QPen(QColor(200,200,200,static_cast<int>(m_opacity*255)),1));
			p.drawText(sx, sy, cnt);
		}
	}
}

void ImageInspectorOverlay::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	m_pixel_scale = computeUiPixelScale();
	update();
}
