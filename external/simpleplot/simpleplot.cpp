// SimplePlot implementation. See simpleplot.h for the API overview.

#include "simpleplot.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QtMath>
#include <algorithm>
#include <cmath>

// ===========================================================================
// SimpleAxis
// ===========================================================================
SimpleAxis::SimpleAxis(AxisType type, SimplePlot *plot) : mType(type), mPlot(plot) {}

void SimpleAxis::setRange(double lower, double upper) {
	if (lower == mRange.lower && upper == mRange.upper) return;
	mRange.lower = lower;
	mRange.upper = upper;
	if (mPlot) mPlot->replot();
}

void SimpleAxis::setRange(const SimpleRange &range) { setRange(range.lower, range.upper); }

void SimpleAxis::setRangeLower(double lower) { setRange(lower, mRange.upper); }
void SimpleAxis::setRangeUpper(double upper) { setRange(mRange.lower, upper); }

void SimpleAxis::setVisible(bool on) {
	if (mVisible == on) return;
	mVisible = on;
	if (mPlot) mPlot->replot();
}

void SimpleAxis::setTickLabels(bool on) {
	if (mTickLabels == on) return;
	mTickLabels = on;
	if (mPlot) mPlot->replot();
}

void SimpleAxis::setShowEndLabel(bool on) {
	if (mShowEndLabel == on) return;
	mShowEndLabel = on;
	if (mPlot) mPlot->replot();
}

void SimpleAxis::setAutoTickCount(int approximateCount) {
	mAutoTickCount = qMax(2, approximateCount);
	if (mPlot) mPlot->replot();
}

void SimpleAxis::setBasePen(const QPen &pen) { mBasePen = pen; if (mPlot) mPlot->replot(); }
void SimpleAxis::setTickPen(const QPen &pen) { mTickPen = pen; if (mPlot) mPlot->replot(); }
void SimpleAxis::setSubTickPen(const QPen &pen) { mSubTickPen = pen; if (mPlot) mPlot->replot(); }
void SimpleAxis::setTickLabelColor(const QColor &color) { mTickLabelColor = color; if (mPlot) mPlot->replot(); }
void SimpleAxis::setLabel(const QString &label) { mLabel = label; if (mPlot) mPlot->replot(); }

// ===========================================================================
// SimpleGraph
// ===========================================================================
void SimpleGraph::setData(const QVector<double> &keys, const QVector<double> &values) {
	const int n = qMin(keys.size(), values.size());
	mKeys = keys.mid(0, n);
	mValues = values.mid(0, n);
	if (mPlot) mPlot->replot();
}

void SimpleGraph::addData(double key, double value) {
	mKeys.append(key);
	mValues.append(value);
	if (mPlot) mPlot->replot();
}

void SimpleGraph::clearData() {
	mKeys.clear();
	mValues.clear();
	if (mPlot) mPlot->replot();
}

void SimpleGraph::setPen(const QPen &pen) { mPen = pen; if (mPlot) mPlot->replot(); }

void SimpleGraph::setScatterShape(ScatterShape shape, double size) {
	mScatterShape = shape;
	mScatterSize = size;
	if (mPlot) mPlot->replot();
}

void SimpleGraph::rescaleKeyAxis(bool onlyEnlarge) {
	if (mKeys.isEmpty() || !mPlot || !mPlot->xAxis) return;
	double lo = mKeys.first(), hi = mKeys.first();
	for (double k : mKeys) { lo = qMin(lo, k); hi = qMax(hi, k); }
	if (lo == hi) { hi = lo + 1.0; }
	SimpleRange cur = mPlot->xAxis->range();
	if (onlyEnlarge) { lo = qMin(lo, cur.lower); hi = qMax(hi, cur.upper); }
	mPlot->xAxis->setRange(lo, hi);
}

void SimpleGraph::rescaleValueAxis(bool onlyEnlarge) {
	if (mValues.isEmpty() || !mPlot || !mPlot->yAxis) return;
	double lo = mValues.first(), hi = mValues.first();
	for (double v : mValues) { lo = qMin(lo, v); hi = qMax(hi, v); }
	if (lo == hi) { lo -= 1.0; hi += 1.0; }
	SimpleRange cur = mPlot->yAxis->range();
	if (onlyEnlarge) { lo = qMin(lo, cur.lower); hi = qMax(hi, cur.upper); }
	mPlot->yAxis->setRange(lo, hi);
}

// ===========================================================================
// SimpleTarget
// ===========================================================================
void SimpleTarget::addSample(double x, double y) {
	mXs.append(x);
	mYs.append(y);
	if (mHistorySize > 0) {
		while (mXs.size() > mHistorySize) { mXs.removeFirst(); mYs.removeFirst(); }
	}
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setData(const QVector<double> &xs, const QVector<double> &ys) {
	const int n = qMin(xs.size(), ys.size());
	mXs = xs.mid(0, n);
	mYs = ys.mid(0, n);
	if (mHistorySize > 0) {
		while (mXs.size() > mHistorySize) { mXs.removeFirst(); mYs.removeFirst(); }
	}
	if (mPlot) mPlot->replot();
}

void SimpleTarget::clear() {
	mXs.clear();
	mYs.clear();
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setHistorySize(int n) {
	mHistorySize = qMax(0, n);
	if (mHistorySize > 0) {
		while (mXs.size() > mHistorySize) { mXs.removeFirst(); mYs.removeFirst(); }
	}
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setRadius(double radius) {
	mRadius = qMax(1e-6, radius);
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setAutoScale(bool on) { mAutoScale = on; if (mPlot) mPlot->replot(); }

void SimpleTarget::setRingCount(int n) { mRingCount = qMax(1, n); if (mPlot) mPlot->replot(); }

void SimpleTarget::setAxisLabels(const QString &horizontal, const QString &vertical) {
	mLabelH = horizontal;
	mLabelV = vertical;
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setUnit(const QString &unit) {
	mUnit = unit;
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setPointSize(double s) {
	mPointSize = qMax(0.0, s);
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setLatestPointSize(double s) {
	mLatestPointSize = qMax(0.0, s);
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setTraceLength(int hops) {
	mTraceLength = qMax(0, hops);
	if (mPlot) mPlot->replot();
}

void SimpleTarget::setNonFadingFraction(double f) {
	mNonFadingFraction = qBound(0.0, f, 1.0);
	if (mPlot) mPlot->replot();
}

double SimpleTarget::rms() const {
	if (mXs.isEmpty()) return 0.0;
	double sum = 0.0;
	for (int i = 0; i < mXs.size(); ++i) sum += mXs[i] * mXs[i] + mYs[i] * mYs[i];
	return std::sqrt(sum / mXs.size());
}

// ===========================================================================
// SimplePlot
// ===========================================================================
SimplePlot::SimplePlot(ChartType type, QWidget *parent) : QWidget(parent), mType(type) {
	if (mType == Graph) {
		xAxis = new SimpleAxis(SimpleAxis::Bottom, this);
		yAxis = new SimpleAxis(SimpleAxis::Left, this);
		xAxis2 = new SimpleAxis(SimpleAxis::Top, this);
		yAxis2 = new SimpleAxis(SimpleAxis::Right, this);
		// Top/right axes are decorative mirrors by default (no labels).
		xAxis2->setTickLabels(false);
		yAxis2->setTickLabels(false);
		xAxis2->setVisible(false);
		yAxis2->setVisible(false);
	} else {
		mTarget = new SimpleTarget(this);
	}
}

SimplePlot::~SimplePlot() {
	clearGraphs();
	delete xAxis;
	delete yAxis;
	delete xAxis2;
	delete yAxis2;
	delete mTarget;
}

void SimplePlot::setBackground(const QBrush &brush) {
	mBackground = brush;
	update();
}

void SimplePlot::replot() { update(); }

SimpleGraph *SimplePlot::addGraph() {
	if (mType != Graph) return nullptr;
	SimpleGraph *g = new SimpleGraph(this);
	// Cycle through a couple of sensible default colours.
	static const QColor palette[] = {QColor(255, 0, 0), QColor(3, 172, 240),
	                                  QColor(0, 200, 0), QColor(241, 183, 1)};
	g->mPen = QPen(palette[mGraphs.size() % 4]);
	mGraphs.append(g);
	return g;
}

SimpleGraph *SimplePlot::graph(int index) const {
	if (index < 0 || index >= mGraphs.size()) return nullptr;
	return mGraphs[index];
}

void SimplePlot::clearGraphs() {
	qDeleteAll(mGraphs);
	mGraphs.clear();
}

QRect SimplePlot::plotArea() const {
	// Margins leave room for tick labels / axis captions.
	const int left = 46, right = 12, top = 10, bottom = 26;
	QRect r = rect().adjusted(left, top, -right, -bottom);
	if (r.width() < 10) r.setWidth(10);
	if (r.height() < 10) r.setHeight(10);
	return r;
}

void SimplePlot::paintEvent(QPaintEvent *) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.fillRect(rect(), mBackground);
	if (mType == Graph) paintGraph(p);
	else paintTarget(p);
}

// --- "nice number" tick step, like most plotting libraries -----------------
static double niceTickStep(double range, int targetCount) {
	if (range <= 0 || targetCount < 1) return 1.0;
	const double rough = range / targetCount;
	const double mag = std::pow(10.0, std::floor(std::log10(rough)));
	const double norm = rough / mag;
	double nice;
	if (norm < 1.5) nice = 1.0;
	else if (norm < 3.0) nice = 2.0;
	else if (norm < 7.0) nice = 5.0;
	else nice = 10.0;
	return nice * mag;
}

static QString fmtTick(double v, double step) {
	int decimals = 0;
	if (step < 1.0) decimals = qMin(4, (int)std::ceil(-std::log10(step)));
	return QString::number(v, 'f', decimals);
}

// Number of sub-intervals a major step is divided into for sub-ticks,
// chosen from the step's leading digit (1->5, 2->4, 5->5) to match the
// spacing QCustomPlot produces.
static int subTickDivisions(double step) {
	if (step <= 0) return 5;
	const double mag = std::pow(10.0, std::floor(std::log10(step)));
	const int lead = (int)std::lround(step / mag); // 1, 2, 5 or 10
	if (lead == 2) return 4;
	return 5;
}

void SimplePlot::paintGraph(QPainter &p) {
	const QRect area = plotArea();
	const SimpleRange xr = xAxis->range();
	const SimpleRange yr = yAxis->range();
	const double xspan = (xr.size() != 0.0) ? xr.size() : 1.0;
	const double yspan = (yr.size() != 0.0) ? yr.size() : 1.0;

	auto mapX = [&](double x) {
		return area.left() + (x - xr.lower) / xspan * area.width();
	};
	auto mapY = [&](double y) {
		return area.bottom() - (y - yr.lower) / yspan * area.height();
	};

	// --- grid + ticks -----------------------------------------------------
	const double xstep = niceTickStep(xspan, xAxis->autoTickCount());
	const double ystep = niceTickStep(yspan, yAxis->autoTickCount());
	QPen gridPen(QColor(255, 255, 255, 50));
	gridPen.setWidth(0);
	// The x == 0 / y == 0 grid lines are drawn more opaque so the origin stands out.
	QPen zeroPen(QColor(255, 255, 255, 120));
	zeroPen.setWidth(0);

	QFont f = p.font();
	f.setPointSizeF(qMax(7.0, f.pointSizeF() - 1.0));
	p.setFont(f);
	QFontMetrics fm(f);

	// Grid, frame and tick marks are all horizontal/vertical lines: draw them
	// with antialiasing OFF so they stay crisp 1px and never bleed a faint halo
	// outside the frame (text labels keep their own TextAntialiasing hint).
	// Antialiasing is restored before the data curves are drawn.
	p.setRenderHint(QPainter::Antialiasing, false);

	// vertical grid lines + x tick labels
	const double xStart = std::ceil(xr.lower / xstep) * xstep;
	double xLastTick = xr.lower - xstep; // last labelled tick value
	for (double x = xStart; x <= xr.upper + xstep * 1e-6; x += xstep) {
		const double px = mapX(x);
		p.setPen(std::fabs(x) < xstep * 1e-6 ? zeroPen : gridPen);
		p.drawLine(QPointF(px, area.top()), QPointF(px, area.bottom()));
		if (xAxis->visible() && xAxis->tickLabels()) {
			p.setPen(xAxis->tickLabelColor());
			const QString s = fmtTick(x, xstep);
			p.drawText(QRectF(px - 40, area.bottom() + 3, 80, 16),
			           Qt::AlignHCenter | Qt::AlignTop, s);
		}
		xLastTick = x;
	}
	// optional label at the range end (e.g. the latest sample index)
	if (xAxis->visible() && xAxis->tickLabels() && xAxis->showEndLabel() &&
	    xr.upper - xLastTick > xstep * 0.3) {
		const double px = mapX(xr.upper);
		// a matching major tick so the label has something to sit under
		p.setPen(xAxis->tickPen());
		p.drawLine(QPointF(px, area.bottom()), QPointF(px, area.bottom() - 5.0));
		// label centred directly on the end tick (same as regular ticks)
		p.setPen(xAxis->tickLabelColor());
		p.drawText(QRectF(px - 40, area.bottom() + 3, 80, 16),
		           Qt::AlignHCenter | Qt::AlignTop, fmtTick(xr.upper, xstep));
	}

	// horizontal grid lines + y tick labels
	const double yStart = std::ceil(yr.lower / ystep) * ystep;
	double yLastTick = yr.lower - ystep;
	for (double y = yStart; y <= yr.upper + ystep * 1e-6; y += ystep) {
		const double py = mapY(y);
		p.setPen(std::fabs(y) < ystep * 1e-6 ? zeroPen : gridPen);
		p.drawLine(QPointF(area.left(), py), QPointF(area.right(), py));
		if (yAxis->visible() && yAxis->tickLabels()) {
			p.setPen(yAxis->tickLabelColor());
			const QString s = fmtTick(y, ystep);
			p.drawText(QRectF(0, py - 8, area.left() - 4, 16),
			           Qt::AlignRight | Qt::AlignVCenter, s);
		}
		yLastTick = y;
	}
	if (yAxis->visible() && yAxis->tickLabels() && yAxis->showEndLabel() &&
	    yr.upper - yLastTick > ystep * 0.3) {
		const double py = mapY(yr.upper);
		double ry = qBound(0.0, py - 8.0, double(height()) - 16.0);
		p.setPen(yAxis->tickLabelColor());
		p.drawText(QRectF(0, ry, area.left() - 4, 16),
		           Qt::AlignRight | Qt::AlignVCenter, fmtTick(yr.upper, ystep));
	}

	// --- axis frame -------------------------------------------------------
	auto drawAxisLine = [&](SimpleAxis *ax) {
		if (!ax || !ax->visible()) return;
		p.setPen(ax->basePen());
		switch (ax->axisType()) {
		case SimpleAxis::Bottom: p.drawLine(area.bottomLeft(), area.bottomRight()); break;
		case SimpleAxis::Top:    p.drawLine(area.topLeft(), area.topRight()); break;
		case SimpleAxis::Left:   p.drawLine(area.topLeft(), area.bottomLeft()); break;
		case SimpleAxis::Right:  p.drawLine(area.topRight(), area.bottomRight()); break;
		}
	};
	drawAxisLine(xAxis);
	drawAxisLine(yAxis);
	drawAxisLine(xAxis2);
	drawAxisLine(yAxis2);

	// --- tick marks -------------------------------------------------------
	// Drawn inward from each visible axis line: long major ticks plus shorter
	// sub-ticks, using the axis tick / sub-tick pens.
	const double majLen = 5.0, subLen = 2.5;

	// vertical ticks (along x) for the bottom (dir +1 => up) and top axes
	auto drawXTicks = [&](SimpleAxis *ax, double yBase, double inward) {
		if (!ax || !ax->visible()) return;
		const double subStep = xstep / subTickDivisions(xstep);
		// Hairline (cosmetic) pens keep the tick marks thin and crisp.
		QPen subPen = ax->subTickPen(); subPen.setWidthF(0.0);
		QPen majPen = ax->tickPen();    majPen.setWidthF(0.0);
		p.setPen(subPen);
		for (double x = std::ceil(xr.lower / subStep) * subStep;
		     x <= xr.upper + subStep * 1e-6; x += subStep) {
			const double px = mapX(x);
			p.drawLine(QPointF(px, yBase), QPointF(px, yBase + inward * subLen));
		}
		p.setPen(majPen);
		for (double x = xStart; x <= xr.upper + xstep * 1e-6; x += xstep) {
			const double px = mapX(x);
			p.drawLine(QPointF(px, yBase), QPointF(px, yBase + inward * majLen));
		}
	};
	// Clip the tick marks to the plot area so they can never bleed outside the
	// frame, independent of antialiasing / device-pixel snapping on any backend.
	p.save();
	p.setClipRect(area);
	drawXTicks(xAxis, area.bottom(), -1.0);
	drawXTicks(xAxis2, area.top(), +1.0);

	// horizontal ticks (along y) for the left (dir +1 => right) and right axes
	auto drawYTicks = [&](SimpleAxis *ax, double xBase, double inward) {
		if (!ax || !ax->visible()) return;
		const double subStep = ystep / subTickDivisions(ystep);
		// Hairline (cosmetic) pens keep the tick marks thin and crisp.
		QPen subPen = ax->subTickPen(); subPen.setWidthF(0.0);
		QPen majPen = ax->tickPen();    majPen.setWidthF(0.0);
		p.setPen(subPen);
		for (double y = std::ceil(yr.lower / subStep) * subStep;
		     y <= yr.upper + subStep * 1e-6; y += subStep) {
			const double py = mapY(y);
			p.drawLine(QPointF(xBase, py), QPointF(xBase + inward * subLen, py));
		}
		p.setPen(majPen);
		for (double y = yStart; y <= yr.upper + ystep * 1e-6; y += ystep) {
			const double py = mapY(y);
			p.drawLine(QPointF(xBase, py), QPointF(xBase + inward * majLen, py));
		}
	};
	drawYTicks(yAxis, area.left(), +1.0);
	drawYTicks(yAxis2, area.right(), -1.0);
	p.restore();

	// --- data -------------------------------------------------------------
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setClipRect(area);
	for (SimpleGraph *g : mGraphs) {
		if (g->mKeys.isEmpty()) continue;

		// connecting line
		if (g->mLineStyle != SimpleGraph::None) {
			p.setPen(g->mPen);
			QPainterPath path;
			bool started = false;
			for (int i = 0; i < g->mKeys.size(); ++i) {
				const QPointF pt(mapX(g->mKeys[i]), mapY(g->mValues[i]));
				if (!started) { path.moveTo(pt); started = true; }
				else {
					switch (g->mLineStyle) {
					case SimpleGraph::StepLeft:
						path.lineTo(QPointF(path.currentPosition().x(), pt.y()));
						path.lineTo(pt); break;
					case SimpleGraph::StepRight:
						path.lineTo(QPointF(pt.x(), path.currentPosition().y()));
						path.lineTo(pt); break;
					case SimpleGraph::StepCenter: {
						const double midx = (path.currentPosition().x() + pt.x()) / 2.0;
						path.lineTo(QPointF(midx, path.currentPosition().y()));
						path.lineTo(QPointF(midx, pt.y()));
						path.lineTo(pt); break;
					}
					default: path.lineTo(pt); break;
					}
				}
			}
			p.drawPath(path);
		}

		// scatter markers
		if (g->mScatterShape != SimpleGraph::NoScatter) {
			const double s = g->mScatterSize;
			p.setPen(g->mPen);
			p.setBrush(g->mScatterShape == SimpleGraph::Disc ? QBrush(g->mPen.color())
			                                              : Qt::NoBrush);
			for (int i = 0; i < g->mKeys.size(); ++i) {
				const QPointF c(mapX(g->mKeys[i]), mapY(g->mValues[i]));
				switch (g->mScatterShape) {
				case SimpleGraph::Disc:
				case SimpleGraph::Circle:
					p.drawEllipse(c, s / 2, s / 2); break;
				case SimpleGraph::Square:
					p.drawRect(QRectF(c.x() - s / 2, c.y() - s / 2, s, s)); break;
				case SimpleGraph::Cross:
					p.drawLine(QPointF(c.x() - s / 2, c.y() - s / 2), QPointF(c.x() + s / 2, c.y() + s / 2));
					p.drawLine(QPointF(c.x() - s / 2, c.y() + s / 2), QPointF(c.x() + s / 2, c.y() - s / 2)); break;
				case SimpleGraph::Plus:
					p.drawLine(QPointF(c.x() - s / 2, c.y()), QPointF(c.x() + s / 2, c.y()));
					p.drawLine(QPointF(c.x(), c.y() - s / 2), QPointF(c.x(), c.y() + s / 2)); break;
				default: break;
				}
			}
		}
	}
	p.setClipping(false);
}

void SimplePlot::paintTarget(QPainter &p) {
	if (!mTarget) return;
	const QRect r = rect();
	const QPointF center(r.center());
	const double pixR = qMax(10.0, qMin(r.width(), r.height()) / 2.0 - 8.0);

	// Determine the data radius represented by the outer ring.
	double dataR = mTarget->mRadius;
	if (mTarget->mAutoScale) {
		double maxR = 0.0;
		for (int i = 0; i < mTarget->mXs.size(); ++i)
			maxR = qMax(maxR, std::hypot(mTarget->mXs[i], mTarget->mYs[i]));
		if (maxR > 0) {
			// round up to a nice multiple of the ring count
			const double step = niceTickStep(maxR, mTarget->mRingCount);
			dataR = step * mTarget->mRingCount;
		}
	}
	if (dataR <= 0) dataR = 1.0;
	const double scale = pixR / dataR;

	// --- concentric rings -------------------------------------------------
	QFont f = p.font();
	f.setPointSizeF(qMax(7.0, f.pointSizeF() - 1.0));
	p.setFont(f);

	p.setBrush(Qt::NoBrush);
	for (int i = 1; i <= mTarget->mRingCount; ++i) {
		const double rr = pixR * i / mTarget->mRingCount;
		p.setPen(mTarget->mRingPen);
		p.drawEllipse(center, rr, rr);
		// ring radius label, placed up-right along the diagonal
		p.setPen(QColor(170, 170, 170));
		const double val = dataR * i / mTarget->mRingCount;
		const QString s = fmtTick(val, niceTickStep(dataR, mTarget->mRingCount)) + mTarget->mUnit;
		p.drawText(QPointF(center.x() + rr * 0.7071 + 2,
		                   center.y() - rr * 0.7071 - 2), s);
	}

	// --- crosshair + diagonals -------------------------------------------
	QPen cross = mTarget->mCrosshairPen;
	p.setPen(cross);
	p.drawLine(QPointF(center.x() - pixR, center.y()), QPointF(center.x() + pixR, center.y()));
	p.drawLine(QPointF(center.x(), center.y() - pixR), QPointF(center.x(), center.y() + pixR));
	QPen diag = cross;
	QColor dc = diag.color(); dc.setAlpha(70); diag.setColor(dc);
	p.setPen(diag);
	const double d = pixR * 0.7071;
	p.drawLine(QPointF(center.x() - d, center.y() - d), QPointF(center.x() + d, center.y() + d));
	p.drawLine(QPointF(center.x() - d, center.y() + d), QPointF(center.x() + d, center.y() - d));

	// Axis captions, placed just inside the outer ring (in the outermost gap)
	// and off the crosshairs so they don't sit on top of the ring circles.
	p.setPen(QColor(220, 220, 220));
	p.drawText(QRectF(center.x() + pixR - 38, center.y() + 3, 30, 16),
	           Qt::AlignRight | Qt::AlignTop, mTarget->mLabelH);
	p.drawText(QRectF(center.x() + 4, center.y() - pixR + 5, 40, 16),
	           Qt::AlignLeft | Qt::AlignTop, mTarget->mLabelV);

	// --- samples ----------------------------------------------------------
	const int n = mTarget->mXs.size();
	const double ps = mTarget->mPointSize;
	for (int i = 0; i < n; ++i) {
		const QPointF c(center.x() + mTarget->mXs[i] * scale,
		                center.y() - mTarget->mYs[i] * scale);
		// fade: the most recent nonFadingFraction of points are full opacity;
		// the older ones fade linearly from 25% (oldest) up to full. Newest is
		// also larger.
		const double age = (n > 1) ? double(i) / (n - 1) : 1.0;
		const double fadeFrac = 1.0 - mTarget->mNonFadingFraction;  // older portion that fades
		QColor col = mTarget->mPointColor;
		col.setAlphaF(age >= fadeFrac ? 1.0 : 0.25 + 0.75 * (age / fadeFrac));
		const bool latest = (i == n - 1);
		if (latest) col = mTarget->mLatestPointColor;
		p.setPen(Qt::NoPen);
		p.setBrush(col);
		const double sz = latest ? mTarget->mLatestPointSize : ps;
		p.drawEllipse(c, sz / 2, sz / 2);
	}

	// connect the most recent mTraceLength hops with a trail
	if (n >= 2 && mTarget->mTraceLength >= 1) {
		p.setPen(QPen(mTarget->mTrailColor, 1.0));
		p.setBrush(Qt::NoBrush);
		QPainterPath path;
		const int from = qMax(0, n - 1 - mTarget->mTraceLength);
		path.moveTo(QPointF(center.x() + mTarget->mXs[from] * scale,
		                    center.y() - mTarget->mYs[from] * scale));
		for (int i = from + 1; i < n; ++i)
			path.lineTo(QPointF(center.x() + mTarget->mXs[i] * scale,
			                    center.y() - mTarget->mYs[i] * scale));
		p.drawPath(path);
	}
}
