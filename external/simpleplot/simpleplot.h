// SimplePlot — a small, fast, standalone Qt widget for two chart types:
// a line/strip "Graph" and a polar "Target" scatter. The public API is kept
// close to QCustomPlot so existing code ports with minimal changes. Everything
// is custom-painted in a single paintEvent; there is no scene graph and no
// per-item QObject overhead.
//
// See API.md for the full reference. Implementation in simpleplot.cpp.

#ifndef SIMPLEPLOT_H
#define SIMPLEPLOT_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QRect>
#include <QStringList>

class QPainter;
class QPaintEvent;

class SimplePlot;

// ===========================================================================
// SimpleRange — a [lower, upper] interval (cf. QCPRange)
// ===========================================================================
struct SimpleRange {
	double lower;
	double upper;

	SimpleRange() : lower(0.0), upper(1.0) {}
	SimpleRange(double l, double u) : lower(l), upper(u) {}

	double size() const { return upper - lower; }
	double center() const { return (lower + upper) / 2.0; }
	bool contains(double v) const { return v >= lower && v <= upper; }
};

// ===========================================================================
// SimpleAxis — one of the four axes of a Graph chart (cf. QCPAxis)
// ===========================================================================
class SimpleAxis {
public:
	enum AxisType { Left, Right, Top, Bottom };

	// Range
	void setRange(double lower, double upper);
	void setRange(const SimpleRange &range);
	void setRangeLower(double lower);
	void setRangeUpper(double upper);
	SimpleRange range() const { return mRange; }
	AxisType axisType() const { return mType; }

	// Visibility & labels
	void setVisible(bool on);
	bool visible() const { return mVisible; }
	void setTickLabels(bool on);
	bool tickLabels() const { return mTickLabels; }
	void setShowEndLabel(bool on);
	bool showEndLabel() const { return mShowEndLabel; }
	void setLabel(const QString &label);
	QString label() const { return mLabel; }

	// Ticks
	void setAutoTickCount(int approximateCount);
	void setTickCount(int approximateCount) { setAutoTickCount(approximateCount); }
	int autoTickCount() const { return mAutoTickCount; }

	// Pens & colours
	void setBasePen(const QPen &pen);
	void setTickPen(const QPen &pen);
	void setSubTickPen(const QPen &pen);
	void setTickLabelColor(const QColor &color);
	void setLabelColor(const QColor &color) { setTickLabelColor(color); }
	QPen basePen() const { return mBasePen; }
	QPen tickPen() const { return mTickPen; }
	QPen subTickPen() const { return mSubTickPen; }
	QColor tickLabelColor() const { return mTickLabelColor; }

private:
	friend class SimplePlot;
	SimpleAxis(AxisType type, SimplePlot *plot);

	AxisType mType;
	SimplePlot *mPlot;
	SimpleRange mRange;                          // 0..1
	bool mVisible = true;
	bool mTickLabels = true;
	bool mShowEndLabel = false;
	int mAutoTickCount = 5;
	QPen mBasePen{QColor(150, 150, 150)};
	QPen mTickPen{QColor(150, 150, 150)};
	QPen mSubTickPen{QColor(150, 150, 150)};
	QColor mTickLabelColor{QColor(255, 255, 255)};
	QString mLabel;
};

// ===========================================================================
// SimpleGraph — one data series of a Graph chart (cf. QCPGraph)
// ===========================================================================
class SimpleGraph {
public:
	enum LineStyle { Line, StepLeft, StepRight, StepCenter, None };
	enum ScatterShape { NoScatter, Disc, Circle, Square, Cross, Plus };

	// Data
	void setData(const QVector<double> &keys, const QVector<double> &values);
	void addData(double key, double value);
	void clearData();
	int dataCount() const { return mKeys.size(); }
	const QVector<double> &keys() const { return mKeys; }
	const QVector<double> &values() const { return mValues; }

	// Appearance
	void setPen(const QPen &pen);
	QPen pen() const { return mPen; }
	void setLineStyle(LineStyle style);
	LineStyle lineStyle() const { return mLineStyle; }
	void setScatterShape(ScatterShape shape, double size = 5.0);
	ScatterShape scatterShape() const { return mScatterShape; }
	void setName(const QString &name);
	QString name() const { return mName; }

	// Auto-scaling
	void rescaleKeyAxis(bool onlyEnlarge = false);
	void rescaleValueAxis(bool onlyEnlarge = false);

private:
	friend class SimplePlot;
	explicit SimpleGraph(SimplePlot *plot) : mPlot(plot) {}

	SimplePlot *mPlot;
	QVector<double> mKeys;
	QVector<double> mValues;
	QPen mPen;
	LineStyle mLineStyle = Line;
	ScatterShape mScatterShape = NoScatter;
	double mScatterSize = 5.0;
	QString mName;
};

// ===========================================================================
// SimpleTarget — the model behind a Target chart
// ===========================================================================
class SimpleTarget {
public:
	// Samples
	void addSample(double x, double y);
	void setData(const QVector<double> &xs, const QVector<double> &ys);
	void clear();
	int sampleCount() const { return mXs.size(); }
	double rms() const;

	// History & scale
	void setHistorySize(int n);
	int historySize() const { return mHistorySize; }
	void setRadius(double radius);
	double radius() const { return mRadius; }
	void setAutoScale(bool on);
	bool autoScale() const { return mAutoScale; }

	// Rings & appearance
	void setRingCount(int n);
	int ringCount() const { return mRingCount; }
	void setRingPen(const QPen &pen);
	void setCrosshairPen(const QPen &pen);
	void setPointColor(const QColor &c);
	void setTrailColor(const QColor &c);
	QColor trailColor() const { return mTrailColor; }
	void setLatestPointColor(const QColor &c);
	QColor latestPointColor() const { return mLatestPointColor; }
	void setPointSize(double s);
	double pointSize() const { return mPointSize; }
	void setLatestPointSize(double s);
	double latestPointSize() const { return mLatestPointSize; }
	void setTraceLength(int hops);
	int traceLength() const { return mTraceLength; }
	void setNonFadingFraction(double f);
	double nonFadingFraction() const { return mNonFadingFraction; }
	void setAxisLabels(const QString &horizontal, const QString &vertical);
	void setUnit(const QString &unit);
	QString unit() const { return mUnit; }

private:
	friend class SimplePlot;
	explicit SimpleTarget(SimplePlot *plot) : mPlot(plot) {}

	SimplePlot *mPlot;
	QVector<double> mXs;
	QVector<double> mYs;
	int mHistorySize = 150;
	double mRadius = 4.0;
	bool mAutoScale = false;
	int mRingCount = 4;
	QPen mRingPen{QColor(120, 120, 120)};
	QPen mCrosshairPen{QColor(150, 150, 150)};
	//QColor mPointColor{QColor(255, 215, 0)};
	QColor mPointColor{QColor(230, 255, 0)};
	QColor mTrailColor{QColor(255, 215, 0, 90)};
	QColor mLatestPointColor{QColor(255, 50, 50)};
	double mPointSize = 3.0;
	double mLatestPointSize = 5.0;
	int mTraceLength = 3;          // number of connecting hops in the history trail
	double mNonFadingFraction = 0.3;  // fraction of newest points drawn at full opacity
	QString mLabelH{QStringLiteral("RA")};
	QString mLabelV{QStringLiteral("Dec")};
	QString mUnit;
};

// ===========================================================================
// SimplePlot — the widget (cf. QCustomPlot)
// ===========================================================================
class SimplePlot : public QWidget {
public:
	enum ChartType { Graph, Target };

	explicit SimplePlot(ChartType type = Graph, QWidget *parent = nullptr);
	~SimplePlot() override;

	ChartType chartType() const { return mType; }

	// Common
	void setBackground(const QBrush &brush);
	QBrush background() const { return mBackground; }
	void replot();
	void setCustomXAxisTicks(const QVector<double> &positions, const QStringList &labels);
	void clearCustomXAxisTicks();

	// Plot-area margins in px, to leave room for the axis captions/tick labels
	// where needed (Graph only).
	void setPlotMargins(int left, int top, int right, int bottom);
	int marginLeft() const { return mMarginLeft; }
	int marginTop() const { return mMarginTop; }
	int marginRight() const { return mMarginRight; }
	int marginBottom() const { return mMarginBottom; }

	// Graph chart members (nullptr on a Target plot)
	SimpleAxis *xAxis = nullptr;    // bottom (primary key axis)
	SimpleAxis *yAxis = nullptr;    // left   (primary value axis)
	SimpleAxis *xAxis2 = nullptr;   // top    (mirrors xAxis range)
	SimpleAxis *yAxis2 = nullptr;   // right  (mirrors yAxis range)

	SimpleGraph *addGraph();
	SimpleGraph *graph(int index) const;
	int graphCount() const { return mGraphs.size(); }
	void clearGraphs();

	// Target chart members
	SimpleTarget *target() const { return mTarget; }

protected:
	void paintEvent(QPaintEvent *) override;

private:
	QRect plotArea() const;
	void paintGraph(QPainter &p);
	void paintTarget(QPainter &p);

	ChartType mType;
	QBrush mBackground{QColor(0, 0, 0, 0)};
	QVector<double> mCustomXAxisTickPositions;
	QStringList mCustomXAxisTickLabels;
	int mMarginLeft = 42;
	int mMarginTop = 8;
	int mMarginRight = 8;
	int mMarginBottom = 22;
	QVector<SimpleGraph *> mGraphs;
	SimpleTarget *mTarget = nullptr;
};

// --- small inline setters that need the complete SimplePlot type -----------
inline void SimpleGraph::setLineStyle(LineStyle style) {
	mLineStyle = style;
	if (mPlot) mPlot->replot();
}

inline void SimpleGraph::setName(const QString &name) {
	mName = name;
}

inline void SimpleTarget::setRingPen(const QPen &pen) {
	mRingPen = pen;
	if (mPlot) mPlot->replot();
}

inline void SimpleTarget::setCrosshairPen(const QPen &pen) {
	mCrosshairPen = pen;
	if (mPlot) mPlot->replot();
}

inline void SimpleTarget::setPointColor(const QColor &c) {
	mPointColor = c;
	if (mPlot) mPlot->replot();
}

inline void SimpleTarget::setTrailColor(const QColor &c) {
	mTrailColor = c;
	if (mPlot) mPlot->replot();
}

inline void SimpleTarget::setLatestPointColor(const QColor &c) {
	mLatestPointColor = c;
	if (mPlot) mPlot->replot();
}

#endif // SIMPLEPLOT_H
