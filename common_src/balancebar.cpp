// Copyright (c) 2026 Rumen G. Bogdanovski
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

// version history
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

#include "balancebar.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QLinearGradient>

#include <algorithm>
#include <cmath>

namespace {

const QColor kGreen(87, 164, 101);
const QColor kAmber(190, 155, 70);
const QColor kRed(182, 87, 80);

// std::clamp is C++17; this project targets C++11.
double clamp(double value, double lo, double hi) {
	return std::max(lo, std::min(hi, value));
}

} // namespace

double BalanceBar::toX(const QRectF &track, double value) const {
	const double c = std::max(-m_scale, std::min(m_scale, value));
	return track.left() + (0.5 + c / (2.0 * m_scale)) * track.width();
}

BalanceBar::BalanceBar(Style style, QWidget *parent) : QWidget(parent), m_style(style) {
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setToolTip(QStringLiteral(
		"<b>Correction response</b> — how well the guiding is tuned on this axis.<br><br>"
		"It compares each frame's leftover error with the previous one, and the marker "
		"shows where the guiding currently sits:<br><br>"
		"• <b>Centre (green)</b> — well tuned. The leftover error is just random noise; "
		"there is nothing more the guider can usefully correct.<br>"
		"• <b>Left (over-correcting)</b> — the guider is overshooting and oscillating. "
		"Try lowering aggressiveness or raising the min-move.<br>"
		"• <b>Right (under-correcting)</b> — the error keeps drifting the same way "
		"between frames. Try raising aggressiveness.<br><br>"
		"A slight bias to the right is usually preferable, so judge by the "
		"colour and how far the marker sits from centre. Please note it needs a couple of minutes "
		"of steady guiding after start or parameter change before it becomes meaningful."));
}

void BalanceBar::setStyle(Style style) {
	if (m_style == style) return;
	m_style = style;
	update();
}

void BalanceBar::setValue(double value, bool valid) {
	m_value = value;
	m_valid = valid;
	update();
}

QSize BalanceBar::sizeHint() const {
	return QSize(180, 22);
}

QSize BalanceBar::minimumSizeHint() const {
	return QSize(60, 12);
}

void BalanceBar::setThresholds(double okThreshold, double warnThreshold, double scale) {
	m_okThreshold = okThreshold;
	m_warnThreshold = warnThreshold;
	m_scale = scale;
	update();
}

QColor BalanceBar::statusColor(double value) const {
	const double a = std::fabs(value);
	if (a <= m_okThreshold) return kGreen;
	if (a <= m_warnThreshold) return kAmber;
	return kRed;
}

void BalanceBar::paintEvent(QPaintEvent *) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	const QRectF area = rect().adjusted(1, 1, -1, -1);

	switch (m_style) {
		case Style::Dot:
			paintDot(p, area);
			break;
		case Style::GradientSoft:
			paintGradient(p, area, false);
			break;
		case Style::GradientHard:
			paintGradient(p, area, true);
			break;
	}
}

// Flat track with a central "balanced" band and a round marker whose position
// and colour encode the value. Left = under, right = over.
void BalanceBar::paintDot(QPainter &p, const QRectF &area) {
	const double th = clamp(area.height() * 0.42, 4.0, 12.0);
	const QRectF track(area.left(), area.center().y() - th / 2.0, area.width(), th);

	p.setPen(Qt::NoPen);
	p.setBrush(QColor(70, 70, 74));
	p.drawRoundedRect(track, th / 2.0, th / 2.0);

	const QRectF band(toX(track, -m_okThreshold), track.top(),
	                   toX(track, m_okThreshold) - toX(track, -m_okThreshold), track.height());
	p.setBrush(QColor(55, 95, 60));
	p.drawRoundedRect(band, th / 2.0, th / 2.0);

	p.setPen(QPen(QColor(140, 140, 145), 1.0));
	const double cx = toX(track, 0.0);
	p.drawLine(QPointF(cx, track.top() - 2.0), QPointF(cx, track.bottom() + 2.0));

	if (!m_valid) {
		p.setPen(QColor(150, 150, 150));
		p.drawText(rect(), Qt::AlignCenter, QStringLiteral("n/a"));
		return;
	}
	const double x = toX(track, m_value);
	const double r = clamp(area.height() * 0.34, 5.0, 10.0);
	p.setBrush(statusColor(m_value));
	p.setPen(QPen(QColor(20, 20, 20), 1.0));
	p.drawEllipse(QPointF(x, track.center().y()), r, r);
}

// Diverging red/amber/green gradient track with a zero tick and a needle
// pointing up into the track. With hardEdges the colour bands snap exactly at
// the thresholds; otherwise they blend softly across a small "fuzz" width.
void BalanceBar::paintGradient(QPainter &p, const QRectF &area, bool hardEdges) {
	const double th = clamp(area.height() * 0.34, 3.0, 8.0);
	const double needle = clamp(area.height() * 0.5, 5.0, 11.0);
	const QRectF track(area.left(), area.top(), area.width(), th);

	const double fWarnNeg = 0.5 - m_warnThreshold / (2 * m_scale);
	const double fOkNeg = 0.5 - m_okThreshold / (2 * m_scale);
	const double fOkPos = 0.5 + m_okThreshold / (2 * m_scale);
	const double fWarnPos = 0.5 + m_warnThreshold / (2 * m_scale);
	const double fuzz = hardEdges ? 0.0015 : 0.035;

	QLinearGradient grad(track.topLeft(), track.topRight());
	grad.setColorAt(0.0, kRed);
	grad.setColorAt(fWarnNeg - fuzz, kRed);
	grad.setColorAt(fWarnNeg + fuzz, kAmber);
	grad.setColorAt(fOkNeg - fuzz, kAmber);
	grad.setColorAt(fOkNeg + fuzz, kGreen);
	grad.setColorAt(fOkPos - fuzz, kGreen);
	grad.setColorAt(fOkPos + fuzz, kAmber);
	grad.setColorAt(fWarnPos - fuzz, kAmber);
	grad.setColorAt(fWarnPos + fuzz, kRed);
	grad.setColorAt(1.0, kRed);
	p.setPen(Qt::NoPen);
	p.setBrush(grad);
	p.drawRoundedRect(track, th / 2.0, th / 2.0);

	// Zero tick: a short vertical mark through the track so "balanced" has a
	// clear visual anchor even before looking at the needle position.
	const double cx = toX(track, 0.0);
	p.setPen(QPen(QColor(20, 20, 20, 200), 1.4));
	p.drawLine(QPointF(cx, track.top() + 1.0), QPointF(cx, track.bottom() - 1.0));

	if (!m_valid) {
		p.setPen(QColor(150, 150, 150));
		p.drawText(area.adjusted(0, th, 0, 0), Qt::AlignCenter, QStringLiteral("n/a"));
		return;
	}
	const double x = toX(track, m_value);

	QPainterPath tri;
	tri.moveTo(x, track.bottom() + 1.0);
	tri.lineTo(x - needle * 0.55, track.bottom() + needle);
	tri.lineTo(x + needle * 0.55, track.bottom() + needle);
	tri.closeSubpath();
	p.setPen(QPen(QColor(20, 20, 20, 160), 1.0));
	p.setBrush(QColor(235, 235, 238));
	p.drawPath(tri);
}
