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
