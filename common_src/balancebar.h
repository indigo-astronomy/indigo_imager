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

#ifndef BALANCEBAR_H
#define BALANCEBAR_H

#include <QWidget>
#include <QColor>

// A compact horizontal gauge that visualizes a signed value against
// "balanced / slightly off / clearly off" thresholds — e.g. the lag-1
// autocorrelation of a guiding residual, where 0 means well tuned and
// larger magnitudes mean over- or under-correction.
//
// Three visual styles are available (see Style); pick whichever fits the
// surrounding UI best. The widget is horizontally scalable: track thickness,
// marker size and font all scale with the widget's own size rather than
// relying on a fixed pixel size.
class BalanceBar : public QWidget {
public:
	enum class Style {
		Dot,          // Flat track with a central "balanced" band and a round marker.
		GradientSoft, // Diverging red/amber/green gradient track with a soft threshold
		              // blend, a zero tick, and a needle pointing into the track.
		GradientHard, // Same as GradientSoft but with hard-edged colour bands instead
		              // of a soft blend.
	};

	explicit BalanceBar(Style style = Style::GradientSoft, QWidget *parent = nullptr);

	void setStyle(Style style);
	Style style() const { return m_style; }

	void setValue(double value, bool valid);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	// Configure the thresholds this bar (and statusColor()) use. |value| <=
	// okThreshold is well tuned, up to warnThreshold is slightly off, beyond
	// that is clearly off. scale is the value that maps to the extreme ends
	// of the bar (larger values clamp). Defaults are 0.2 / 0.4 / 0.9.
	void setThresholds(double okThreshold, double warnThreshold, double scale);
	double okThreshold() const { return m_okThreshold; }
	double warnThreshold() const { return m_warnThreshold; }
	double scale() const { return m_scale; }

	QColor statusColor(double value) const;

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	void paintDot(class QPainter &p, const QRectF &area);
	void paintGradient(class QPainter &p, const QRectF &area, bool hardEdges);
	double toX(const QRectF &track, double value) const;

	Style m_style;
	double m_value = 0.0;
	bool m_valid = false;
	double m_okThreshold = 0.2;
	double m_warnThreshold = 0.4;
	double m_scale = 0.9;
};

#endif // BALANCEBAR_H
