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

#include "SchedulerSkyChart.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QToolTip>
#include <cmath>

// MARK: Positions

static const double DEG = M_PI / 180.0;

static double julianDay(qint64 t) {
	return t / 86400.0 + 2440587.5;
}

static double normalize(double degrees) {
	degrees = std::fmod(degrees, 360.0);
	return degrees < 0 ? degrees + 360.0 : degrees;
}

static void eclipticToEquatorial(double longitude, double latitude, double obliquity, double *ra, double *dec) {
	double l = longitude * DEG, b = latitude * DEG, e = obliquity * DEG;
	double x = std::cos(b) * std::cos(l);
	double y = std::cos(e) * std::cos(b) * std::sin(l) - std::sin(e) * std::sin(b);
	double z = std::sin(e) * std::cos(b) * std::sin(l) + std::cos(e) * std::sin(b);
	*ra = normalize(std::atan2(y, x) / DEG) / 15.0;
	*dec = std::asin(z) / DEG;
}

void SchedulerSky::sun(qint64 t, double *ra, double *dec) {
	double n = julianDay(t) - 2451545.0;
	double l = normalize(280.460 + 0.9856474 * n);
	double g = normalize(357.528 + 0.9856003 * n) * DEG;
	double lambda = l + 1.915 * std::sin(g) + 0.020 * std::sin(2 * g);
	eclipticToEquatorial(lambda, 0, 23.439 - 0.0000004 * n, ra, dec);
}

void SchedulerSky::moon(qint64 t, double *ra, double *dec, double *parallax) {
	double n = julianDay(t) - 2451545.0;
	double c = n / 36525.0;
	auto s = [](double degrees) { return std::sin(normalize(degrees) * DEG); };
	auto k = [](double degrees) { return std::cos(normalize(degrees) * DEG); };
	double lambda = 218.32 + 481267.881 * c
		+ 6.29 * s(135.0 + 477198.87 * c) - 1.27 * s(259.3 - 413335.36 * c)
		+ 0.66 * s(235.7 + 890534.22 * c) + 0.21 * s(269.9 + 954397.74 * c)
		- 0.19 * s(357.5 + 35999.05 * c) - 0.11 * s(186.5 + 966404.03 * c);
	double beta = 5.13 * s(93.3 + 483202.02 * c) + 0.28 * s(228.2 + 960400.89 * c)
		- 0.28 * s(318.3 + 6003.15 * c) - 0.17 * s(217.6 - 407332.21 * c);
	*parallax = 0.9508 + 0.0518 * k(134.9 + 477198.85 * c) + 0.0095 * k(259.2 - 413335.38 * c)
		+ 0.0078 * k(235.7 + 890534.23 * c) + 0.0028 * k(269.9 + 954397.70 * c);
	eclipticToEquatorial(lambda, beta, 23.439 - 0.0000004 * n, ra, dec);
}

double SchedulerSky::altitude(double ra, double dec, qint64 t, double latitude, double longitude) {
	double gmst = 280.46061837 + 360.98564736629 * (julianDay(t) - 2451545.0);
	double hour_angle = (gmst + longitude - ra * 15.0) * DEG;
	double phi = latitude * DEG, delta = dec * DEG;
	double sin_altitude = std::sin(phi) * std::sin(delta) + std::cos(phi) * std::cos(delta) * std::cos(hour_angle);
	return std::asin(qBound(-1.0, sin_altitude, 1.0)) / DEG;
}

double SchedulerSky::targetAltitude(double ra, double dec, qint64 t, double latitude, double longitude) {
	double years = (julianDay(t) - 2451545.0) / 365.25;
	double alpha = ra * 15 * DEG, delta = qBound(-89.9, dec, 89.9) * DEG;
	double ra_date = ra + (3.075 + 1.336 * std::sin(alpha) * std::tan(delta)) * years / 3600.0;
	double dec_date = dec + 20.04 * std::cos(alpha) * years / 3600.0;
	return altitude(ra_date, dec_date, t, latitude, longitude);
}

double SchedulerSky::sunAltitude(qint64 t, double latitude, double longitude) {
	double ra, dec;
	sun(t, &ra, &dec);
	return altitude(ra, dec, t, latitude, longitude);
}

double SchedulerSky::moonAltitude(qint64 t, double latitude, double longitude) {
	double ra, dec, parallax;
	moon(t, &ra, &dec, &parallax);
	double geocentric = altitude(ra, dec, t, latitude, longitude);
	return geocentric - parallax * std::cos(geocentric * DEG);
}

QPair<qint64, qint64> SchedulerSky::nightWindow(qint64 now, double latitude, double longitude) {
	const qint64 step = 600;
	const qint64 day = 86400;
	auto dark = [&](qint64 t) { return sunAltitude(t, latitude, longitude) < -0.833; };
	auto whole_hours = [](qint64 start, qint64 end) {
		return qMakePair(start - ((start % 3600) + 3600) % 3600, end + (3600 - ((end % 3600) + 3600) % 3600) % 3600);
	};
	qint64 start = now;
	if (dark(now)) {
		while (start > now - day && dark(start - step)) start -= step;
	} else {
		while (start < now + day && !dark(start)) start += step;
	}
	qint64 end = start;
	while (end < start + day && dark(end)) end += step;
	if (start <= now - day || start >= now + day || end >= start + day) {
		// the Sun does not set or does not rise
		return whole_hours(now - 2 * 3600, now + 22 * 3600);
	}
	// the samples find the sunset up to a step late
	return whole_hours(start - 3600 - step, end + 3600);
}

// MARK: Chart

SchedulerSkyChart::SchedulerSkyChart(QWidget *parent) : QWidget(parent) {
	setObjectName("SchedulerSkyChart");
	setMouseTracking(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_timer = new QTimer(this);
	m_timer->setInterval(60 * 1000);
	connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
	m_timer->start();
}

void SchedulerSkyChart::setSite(bool known, double latitude, double longitude) {
	if (known == m_site_known && latitude == m_latitude && longitude == m_longitude) return;
	m_site_known = known;
	m_latitude = latitude;
	m_longitude = longitude;
	update();
}

void SchedulerSkyChart::setTargets(const QList<Target>& targets) {
	m_targets = targets;
	update();
}

void SchedulerSkyChart::setRuns(const QList<Run>& runs) {
	m_runs = runs;
	update();
}

void SchedulerSkyChart::setHighlighted(const QStringList& names) {
	if (names == m_highlighted) return;
	m_highlighted = names;
	update();
}

void SchedulerSkyChart::setAccentSource(QWidget *widget) {
	m_accent_source = widget;
	update();
}

void SchedulerSkyChart::setNow(qint64 now) {
	m_now = now;
	update();
}

QString SchedulerSkyChart::describe() const {
	if (!m_site_known) {
		return "no site";
	}
	QStringList targets, runs;
	for (const Target& target : m_targets) targets << target.name;
	for (const Run& run : m_runs) runs << run.name;
	return QString("site %1 %2 | targets %3 | runs %4 | highlighted %5")
		.arg(m_latitude, 0, 'f', 1).arg(m_longitude, 0, 'f', 1)
		.arg(targets.join(",")).arg(runs.join(",")).arg(m_highlighted.join(","));
}

qint64 SchedulerSkyChart::now() const {
	return m_now ? m_now : QDateTime::currentSecsSinceEpoch();
}

QSize SchedulerSkyChart::sizeHint() const {
	return QSize(300, 200);
}

QSize SchedulerSkyChart::minimumSizeHint() const {
	return QSize(160, 120);
}

QRect SchedulerSkyChart::plotRect() const {
	QFontMetrics metrics(font());
	int left = metrics.horizontalAdvance("90°") + 8;
	int bottom = metrics.height() + 10;
	return QRect(left, metrics.height() / 2 + 2, width() - left - 6, height() - bottom - metrics.height() / 2 - 2);
}

void SchedulerSkyChart::paintEvent(QPaintEvent *event) {
	Q_UNUSED(event);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	const QPalette& pal = palette();
	QColor text = pal.color(QPalette::WindowText);
	// the sky has the accent of the theme, the curves of the marked tasks a light shade of it
	if (m_accent_source) m_accent_source->ensurePolished();
	QColor accent = (m_accent_source ? m_accent_source->palette() : pal).color(QPalette::Highlight);
	QColor highlight = accent.lightness() < 128 ? accent.lighter(250) : accent;
	QColor faint = text;
	faint.setAlpha(70);

	QRect r = plotRect();
	p.fillRect(r, pal.color(QPalette::Base));
	if (!m_site_known || r.width() < 20 || r.height() < 20) {
		p.setPen(text);
		p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, "The site is not known:\nthe Mount Agent of the scheduler sets it");
		p.setPen(faint);
		p.drawRect(r);
		return;
	}

	qint64 t_now = now();
	QPair<qint64, qint64> window = SchedulerSky::nightWindow(t_now, m_latitude, m_longitude);
	double span = double(window.second - window.first);
	auto x_of = [&](qint64 t) { return r.left() + (t - window.first) / span * r.width(); };
	auto y_of = [&](double altitude) { return r.bottom() - qBound(0.0, altitude, 90.0) / 90.0 * r.height(); };
	auto t_of = [&](double x) { return window.first + qint64((x - r.left()) / r.width() * span); };

	// the daylight: the darker the sky, the less of it
	for (int x = r.left(); x <= r.right(); x += 2) {
		double sun = SchedulerSky::sunAltitude(t_of(x + 1), m_latitude, m_longitude);
		int alpha = sun >= 0 ? 170 : sun >= -6 ? 120 : sun >= -12 ? 75 : sun >= -18 ? 35 : 0;
		if (alpha) {
			QColor sky = accent;
			sky.setAlpha(alpha);
			p.fillRect(QRectF(x, r.top(), qMin(2, r.right() - x + 1), r.height()), sky);
		}
	}

	// the altitude grid and the hours
	p.setPen(QPen(faint, 1, Qt::DotLine));
	for (int altitude : { 30, 60 }) {
		p.drawLine(QPointF(r.left(), y_of(altitude)), QPointF(r.right(), y_of(altitude)));
	}
	QFontMetrics metrics(font());
	p.setPen(text);
	for (int altitude : { 0, 30, 60, 90 }) {
		QRectF label(0, y_of(altitude) - metrics.height() / 2.0, r.left() - 4, metrics.height());
		p.drawText(label, Qt::AlignRight | Qt::AlignVCenter, QString("%1°").arg(altitude));
	}
	int hours = int(span / 3600);
	int label_step = 1;
	while (label_step < 6 && r.width() / double(hours) * label_step < metrics.horizontalAdvance("00") * 2.5) label_step++;
	for (qint64 t = window.first; t <= window.second; t += 3600) {
		double x = x_of(t);
		int hour = int((t / 3600) % 24);
		p.setPen(faint);
		p.drawLine(QPointF(x, r.bottom()), QPointF(x, r.bottom() + 3));
		if (hour % label_step == 0) {
			p.setPen(text);
			QString label = QString("%1").arg(hour, 2, 10, QChar('0'));
			QRectF box(x - 20, r.bottom() + 8, 40, metrics.height());
			p.drawText(box, Qt::AlignHCenter | Qt::AlignTop, label);
		}
	}

	const qint64 sample = 300;
	auto curve = [&](std::function<double(qint64)> altitude_at, qint64 from, qint64 to) {
		QPainterPath path;
		bool drawing = false;
		for (qint64 t = from; t <= to + sample; t += sample) {
			qint64 at = qMin(t, to);
			double altitude = altitude_at(at);
			QPointF point(x_of(at), y_of(altitude));
			if (altitude < 0) {
				if (drawing) path.lineTo(point);
				drawing = false;
			} else if (!drawing) {
				path.moveTo(point);
				drawing = true;
			} else {
				path.lineTo(point);
			}
			if (at == to) break;
		}
		return path;
	};

	p.save();
	p.setClipRect(r.adjusted(0, -1, 0, 1));

	// the Moon
	QColor moon_color = text;
	moon_color.setAlpha(170);
	p.setPen(QPen(moon_color, 1.2, Qt::DashLine));
	p.drawPath(curve([&](qint64 t) { return SchedulerSky::moonAltitude(t, m_latitude, m_longitude); }, window.first, window.second));

	// the targets, the highlighted ones last; a planned run is the thick part of the curve
	QList<const Target*> order;
	for (const Target& target : m_targets) if (!m_highlighted.contains(target.name)) order.append(&target);
	for (const Target& target : m_targets) if (m_highlighted.contains(target.name)) order.append(&target);
	QColor other = text;
	other.setAlpha(150);
	for (const Target *target : order) {
		bool marked = m_highlighted.contains(target->name);
		QColor color = marked ? highlight : other;
		auto altitude_at = [&](qint64 t) { return SchedulerSky::targetAltitude(target->ra, target->dec, t, m_latitude, m_longitude); };
		if (marked && target->minAltitude > 0) {
			QColor limit = highlight;
			limit.setAlpha(140);
			p.setPen(QPen(limit, 1, Qt::DotLine));
			p.drawLine(QPointF(r.left(), y_of(target->minAltitude)), QPointF(r.right(), y_of(target->minAltitude)));
		}
		p.setPen(QPen(color, marked ? 2.0 : 1.2));
		p.drawPath(curve(altitude_at, window.first, window.second));
		for (const Run& run : m_runs) {
			if (run.name != target->name || run.end < window.first || run.start > window.second) continue;
			p.setPen(QPen(color, marked ? 5.0 : 4.0, Qt::SolidLine, Qt::RoundCap));
			p.drawPath(curve(altitude_at, qMax(run.start, window.first), qMin(run.end, window.second)));
		}
		// the name at the highest point
		qint64 best = window.first;
		double best_altitude = -90;
		for (qint64 t = window.first; t <= window.second; t += sample) {
			double altitude = altitude_at(t);
			if (altitude > best_altitude) {
				best_altitude = altitude;
				best = t;
			}
		}
		if (best_altitude > 0 && (marked || m_targets.size() <= 6)) {
			QString name = metrics.elidedText(target->name, Qt::ElideRight, r.width() / 3);
			double width = metrics.horizontalAdvance(name);
			double x = qBound(double(r.left() + 2), x_of(best) - width / 2, r.right() - width - 2);
			double y = qMax(double(r.top() + metrics.ascent() + 1), y_of(best_altitude) - 4);
			p.setPen(color);
			p.drawText(QPointF(x, y), name);
		}
	}
	p.restore();

	// the planned runs of all tasks, under the time axis
	for (const Run& run : m_runs) {
		if (run.end < window.first || run.start > window.second) continue;
		QColor color = m_highlighted.contains(run.name) ? highlight : other;
		double x1 = qMax(double(r.left()), x_of(run.start)), x2 = qMin(double(r.right()), x_of(qMax(run.end, run.start + 60)));
		p.fillRect(QRectF(x1, r.bottom() + 2, qMax(2.0, x2 - x1), 4), color);
	}

	// now
	if (t_now >= window.first && t_now <= window.second) {
		p.setPen(QPen(text, 1));
		p.drawLine(QPointF(x_of(t_now), r.top()), QPointF(x_of(t_now), r.bottom()));
	}
	p.setPen(faint);
	p.setBrush(Qt::NoBrush);
	p.drawRect(r);
}

void SchedulerSkyChart::mouseMoveEvent(QMouseEvent *event) {
	QRect r = plotRect();
	QPoint pos = event->pos();
	if (!m_site_known || !r.contains(pos)) {
		QToolTip::hideText();
		return;
	}
	QPair<qint64, qint64> window = SchedulerSky::nightWindow(now(), m_latitude, m_longitude);
	qint64 t = window.first + qint64(double(pos.x() - r.left()) / r.width() * (window.second - window.first));
	QStringList lines;
	lines << QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString("yyyy-MM-dd HH:mm") + " UTC";
	for (const Target& target : m_targets) {
		double altitude = SchedulerSky::targetAltitude(target.ra, target.dec, t, m_latitude, m_longitude);
		if (altitude > 0) lines << QString("%1: %2°").arg(target.name).arg(altitude, 0, 'f', 0);
	}
	double moon = SchedulerSky::moonAltitude(t, m_latitude, m_longitude);
	lines << QString("Moon: %1").arg(moon > 0 ? QString("%1°").arg(moon, 0, 'f', 0) : QString("below the horizon"));
	lines << QString("Sun: %1°").arg(SchedulerSky::sunAltitude(t, m_latitude, m_longitude), 0, 'f', 0);
	for (const Run& run : m_runs) {
		if (t >= run.start && t <= run.end) lines << QString("Planned: %1").arg(run.name);
	}
	QToolTip::showText(event->globalPos(), lines.join("\n"), this);
}
