#include "inspection_overlay.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <cmath>

InspectionOverlay::InspectionOverlay(QWidget *parent)
	: QWidget(parent), m_center_hfd(0), m_opacity(0.85),
	  m_center_detected(0), m_center_used(0), m_center_rejected(0)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void InspectionOverlay::setInspectionResult(const std::vector<double> &directions, double center_hfd) {
	m_dirs = directions;
	m_center_hfd = center_hfd;
	// clear counts if none provided
	m_detected.clear();
	m_used.clear();
	m_rejected.clear();
	m_center_detected = m_center_used = m_center_rejected = 0;
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
	update();
}

void InspectionOverlay::setInspectionResult(const std::vector<double> &directions, double center_hfd,
										   const std::vector<int> &detected, const std::vector<int> &used, const std::vector<int> &rejected,
										   int center_detected, int center_used, int center_rejected,
										   const std::vector<QPointF> &used_points, const std::vector<double> &used_radii, const std::vector<QPointF> &rejected_points) {
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

	// Base radius scaled to image diagonal
	double base = std::min(imgRect.width(), imgRect.height()) * 0.2;

	// Normalize directions by center_hfd to get relative scale
	double ref = (m_center_hfd > 0) ? m_center_hfd : 1.0;

	QPolygonF poly;
	for (int i = 0; i < 8; ++i) {
		// start at North (up) and proceed clockwise in 45deg steps
		double angle = M_PI_2 - M_PI_4 * i; // angles: 90,45,0,-45,...
		double h = m_dirs[i];
		double scale = (h > 0) ? (h / ref) : 1.0;
		double r = base * scale;
		double x = center.x() + r * std::cos(angle);
		double y = center.y() - r * std::sin(angle); // y inverted for screen coords
		poly << QPointF(x, y);
	}

	// Draw a stylized octagon similar to ASTAP/NINA:
	// 1) soft glow (wide translucent stroke)
	// 2) semi-transparent fill
	// 3) main crisp stroke with rounded joins
	QPainterPath path;
	path.addPolygon(poly);
	path.closeSubpath();

	// soft glow
	QPen glowPen(QColor(255, 0, 255, static_cast<int>(m_opacity * 90)), 10, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	p.setPen(glowPen);
	p.setBrush(Qt::NoBrush);
	p.drawPath(path);

	// semi-transparent fill
	QColor fillCol(255, 0, 255, static_cast<int>(m_opacity * 30));
	p.setPen(Qt::NoPen);
	p.setBrush(fillCol);
	p.drawPath(path);

	// main stroke
	QPen mainPen(QColor(255, 0, 200, static_cast<int>(m_opacity * 230)), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	p.setBrush(Qt::NoBrush);
	p.setPen(mainPen);
	p.drawPath(path);

	// draw small vertex markers to emphasize corners
	for (int i = 0; i < poly.size(); ++i) {
		QPointF v = poly.at(i);
		// white center with magenta border
		p.setBrush(QColor(255,255,255, static_cast<int>(m_opacity*230)));
		p.setPen(QPen(QColor(255,0,200, static_cast<int>(m_opacity*230)), 1.8));
		double vr = std::max(3.0, std::min(width(), height()) * 0.006);
		p.drawEllipse(v, vr, vr);
	}

	// draw center marker
	p.setPen(QPen(QColor(200,200,200,static_cast<int>(m_opacity*255)),1));
	p.drawEllipse(center, 3, 3);

	// draw labels for each octagon vertex (average HFD)
	if (m_dirs.size() >= 8) {
		QFont font = p.font();
		// scale font based on widget size (smaller than before)
		int fontSize = std::max(8, static_cast<int>(std::min(width(), height()) * 0.02));
		font.setPointSize(fontSize);
		font.setBold(true);
		p.setFont(font);
		p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)), 1));

		for (int i = 0; i < 8; ++i) {
			QPointF pt = poly.at(i);
			// direction from center to vertex
			QPointF dir = pt - center;
			double len = std::hypot(dir.x(), dir.y());
			QPointF labelPos = center;
			if (len > 0.1) {
				// place label slightly outside the vertex
				double scale = (len + fontSize * 1.5) / len;
				labelPos = QPointF(center.x() + dir.x() * scale, center.y() + dir.y() * scale);
			} else {
				labelPos = pt;
			}

			QString txt = QString::number(m_dirs[i], 'f', 2) + " px";
			QRectF tb = p.fontMetrics().boundingRect(txt);
			// center the text at labelPos but offset a bit
			QPointF drawPos(labelPos.x() - tb.width()/2.0, labelPos.y() - tb.height()/2.0);
			// draw background for readability
			QColor bg = QColor(0,0,0, static_cast<int>(m_opacity*180));
			p.setBrush(bg);
			p.setPen(Qt::NoPen);
			QRectF bgRect(drawPos.x()-4, drawPos.y()-2, tb.width()+8, tb.height()+4);
			p.drawRoundedRect(bgRect, 3, 3);
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)),1));
			p.drawText(drawPos.x(), drawPos.y() + tb.height(), txt);

			// draw counts if available (smaller font)
			if (!m_detected.empty() && i < static_cast<int>(m_detected.size())) {
				QFont small = p.font();
				int smallSize = std::max(7, static_cast<int>(std::min(width(), height()) * 0.015));
				small.setPointSize(smallSize);
				small.setBold(false);
				p.setFont(small);
				QString cnt = QString("D:%1 U:%2 R:%3").arg(m_detected[i]).arg(m_used[i]).arg(m_rejected[i]);
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
				// restore font for next label
				p.setFont(font);
			}
		}
	}

	// draw center HFD label
	if (m_center_hfd > 0) {
		QFont font = p.font();
		font.setPointSize(std::max(8, static_cast<int>(std::min(width(), height()) * 0.02)));
		font.setBold(true);
		p.setFont(font);
		QString txt = QString("C: %1 px").arg(QString::number(m_center_hfd, 'f', 2));
		QRectF tb = p.fontMetrics().boundingRect(txt);
		QPointF drawPos(center.x() - tb.width()/2.0, center.y() - tb.height()/2.0 - 6);
		QColor bg = QColor(0,0,0, static_cast<int>(m_opacity*180));
		p.setBrush(bg);
		p.setPen(Qt::NoPen);
		QRectF bgRect(drawPos.x()-4, drawPos.y()-2, tb.width()+8, tb.height()+4);
		p.drawRoundedRect(bgRect, 3, 3);
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(QColor(255,255,255,static_cast<int>(m_opacity*255)),1));
		p.drawText(drawPos.x(), drawPos.y() + tb.height(), txt);

		// draw center counts if available
		if (m_center_detected > 0 || m_center_used > 0 || m_center_rejected > 0) {
			QFont small = p.font();
			small.setPointSize(std::max(7, static_cast<int>(std::min(width(), height()) * 0.015)));
			small.setBold(false);
			p.setFont(small);
			QString cnt = QString("D:%1 U:%2 R:%3").arg(m_center_detected).arg(m_center_used).arg(m_center_rejected);
			QRectF tb2 = p.fontMetrics().boundingRect(cnt);
			QPointF drawPos2(center.x() - tb2.width()/2.0, drawPos.y() + tb.height() + 2);
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

	// draw used/rejected star markers
	// used: small green dots; rejected: small red crosses
	p.setRenderHint(QPainter::Antialiasing, true);

	p.setPen(Qt::NoPen);
	for (size_t i = 0; i < m_used_points.size(); ++i) {
		QPointF pview = m_used_points[i];
		double r = (i < m_used_radii.size() ? m_used_radii[i] : std::max(3.0, std::min(width(), height()) * 0.005));
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(QColor(0, 200, 0, static_cast<int>(m_opacity*230)), 1.5));
		p.drawEllipse(pview, r, r);
	}

	for (const QPointF &pt : m_rejected_points) {
		QPointF pview = pt;
		double s = std::max(3.0, static_cast<double>(std::min(width(), height()) * 0.006));
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(QColor(255, 0, 0, static_cast<int>(m_opacity*220)), 1.5));
		p.drawLine(QPointF(pview.x()-s, pview.y()-s), QPointF(pview.x()+s, pview.y()+s));
		p.drawLine(QPointF(pview.x()-s, pview.y()+s), QPointF(pview.x()+s, pview.y()-s));
	}
}
