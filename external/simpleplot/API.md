# SimplePlot API Reference

A small, fast, standalone Qt widget for two chart types — a line/strip
**Graph** and a polar **Target** scatter. The public API is intentionally kept
close to QCustomPlot so existing code ports with minimal changes. Everything is
custom-painted in a single `paintEvent`; there is no scene graph and no
per-item `QObject` overhead.

Header: `simpleplot.h` · Implementation: `simpleplot.cpp` · Requires: Qt 5 or
Qt 6 (`QtWidgets`).

## Contents

- [Overview](#overview)
- [`SimplePlot`](#class-simpleplot) — the widget
- [`SimpleAxis`](#class-simpleaxis) — an axis of a Graph chart
- [`SimpleGraph`](#class-simplegraph) — one line series
- [`SimpleTarget`](#class-simpletarget) — the target scatter model
- [`SimpleRange`](#struct-simplerange) — a `[lower, upper]` interval
- [Examples](#examples)

---

## Overview

```
SimplePlot (QWidget)
├── ChartType::Graph
│     ├── xAxis, yAxis, xAxis2, yAxis2 : SimpleAxis*
│     └── graphs : SimpleGraph*   (addGraph / graph(i))
└── ChartType::Target
      └── target() : SimpleTarget*
```

The chart type is fixed at construction. A `Graph` plot owns four axes and any
number of `SimpleGraph` series; a `Target` plot owns one `SimpleTarget` model.
Accessing the wrong family for the chart type returns `nullptr` (e.g.
`addGraph()` on a Target plot).

All setters that change appearance or data call `replot()` internally, so you
rarely need to call it yourself.

---

## class `SimplePlot`

The widget. Derives from `QWidget`. Construct it with the chart type you want.

```cpp
enum ChartType { Graph, Target };

explicit SimplePlot(ChartType type = Graph, QWidget *parent = nullptr);
```

### Construction & type

| Method | Description |
|--------|-------------|
| `SimplePlot(ChartType type = Graph, QWidget *parent = nullptr)` | Create a plot of the given type. The type cannot be changed later. |
| `ChartType chartType() const` | Returns `Graph` or `Target`. |

### Common

| Method | Description |
|--------|-------------|
| `void setBackground(const QBrush &brush)` | Fill colour/brush of the whole widget. Default is transparent (`QColor(0,0,0,0)`), letting the parent background show through. |
| `QBrush background() const` | Current background brush. |
| `void replot()` | Schedule a repaint (alias of `update()`, named for QCustomPlot source compatibility). |

### Graph chart members

Public axis pointers, exactly like QCustomPlot. Valid only for a `Graph` plot
(they are `nullptr` on a `Target` plot).

| Member | Description |
|--------|-------------|
| `SimpleAxis *xAxis` | Bottom axis (primary key axis). |
| `SimpleAxis *yAxis` | Left axis (primary value axis). |
| `SimpleAxis *xAxis2` | Top axis. Automatically **mirrors** `xAxis`'s range. |
| `SimpleAxis *yAxis2` | Right axis. Automatically **mirrors** `yAxis`'s range. |

Because the secondary axes auto-mirror, the QCustomPlot
`connect(xAxis, rangeChanged, xAxis2, setRange)` wiring is unnecessary — just
call `xAxis2->setVisible(true)` if you want the top/right frame drawn.

| Method | Description |
|--------|-------------|
| `SimpleGraph *addGraph()` | Add a new line series and return it. The pen defaults to a colour cycled from a small palette (red, blue, green, gold). Returns `nullptr` on a Target plot. |
| `SimpleGraph *graph(int index) const` | The series at `index` (0-based), or `nullptr` if out of range. |
| `int graphCount() const` | Number of series. |
| `void clearGraphs()` | Delete all series. |

### Target chart members

| Method | Description |
|--------|-------------|
| `SimpleTarget *target() const` | The target model (valid only for a `Target` plot, otherwise `nullptr`). |

---

## class `SimpleAxis`

One of the four axes of a `Graph` chart (cf. `QCPAxis`). You never construct it
directly — obtain it through `SimplePlot::xAxis` / `yAxis` / `xAxis2` / `yAxis2`.

```cpp
enum AxisType { Left, Right, Top, Bottom };
```

### Range

| Method | Description |
|--------|-------------|
| `void setRange(double lower, double upper)` | Set the visible value range. |
| `void setRange(const SimpleRange &range)` | Set the range from a `SimpleRange`. |
| `void setRangeLower(double lower)` | Change only the lower bound. |
| `void setRangeUpper(double upper)` | Change only the upper bound. |
| `SimpleRange range() const` | Current range. |
| `AxisType axisType() const` | Which of the four axes this is. |

### Visibility & labels

| Method | Description |
|--------|-------------|
| `void setVisible(bool on)` | Show/hide the axis line (and its ticks/labels). |
| `bool visible() const` | Visibility state. |
| `void setTickLabels(bool on)` | Show/hide the **numeric labels** only. Tick marks and grid are unaffected; x and y are independent. |
| `bool tickLabels() const` | Whether numbers are shown. |
| `void setShowEndLabel(bool on)` | Also label the exact upper bound of the range. The "nice number" ticks rarely land on the range end (e.g. a range of `0..59.2`), so the last value is otherwise unlabelled. A matching tick is drawn and the label is auto-skipped if it would collide with the last regular tick. Default `false`. |
| `bool showEndLabel() const` | End-label state. |
| `void setLabel(const QString &label)` | Set an axis caption string (stored; reserved for captions). |
| `QString label() const` | Current axis caption. |

### Ticks

| Method | Description |
|--------|-------------|
| `void setAutoTickCount(int approximateCount)` | Approximate number of major ticks; the actual step is rounded to a "nice" 1/2/5×10ⁿ value. Sub-ticks are derived automatically (1→5, 2→4, 5→5 subdivisions). Minimum 2. |
| `void setTickCount(int approximateCount)` | Alias of `setAutoTickCount`. |
| `int autoTickCount() const` | Current approximate tick count. |

### Pens & colours

| Method | Description |
|--------|-------------|
| `void setBasePen(const QPen &pen)` | Pen for the axis base line. |
| `void setTickPen(const QPen &pen)` | Pen for the major tick marks. |
| `void setSubTickPen(const QPen &pen)` | Pen for the minor (sub) tick marks. |
| `void setTickLabelColor(const QColor &color)` | Colour of the numeric labels. |
| `void setLabelColor(const QColor &color)` | Alias of `setTickLabelColor`. |
| `QPen basePen() const` / `tickPen() const` / `subTickPen() const` | Current pens. |
| `QColor tickLabelColor() const` | Current label colour. |

**Defaults:** base/tick/sub-tick pens are grey `(150,150,150)`; label colour is
white; range is `0..1`; `autoTickCount` is 5; labels on, end-label off.

---

## class `SimpleGraph`

One data series of a `Graph` chart (cf. `QCPGraph`). Created by
`SimplePlot::addGraph()`; never constructed directly.

```cpp
enum LineStyle   { Line, StepLeft, StepRight, StepCenter, None };
enum ScatterShape { NoScatter, Disc, Circle, Square, Cross, Plus };
```

### Data

| Method | Description |
|--------|-------------|
| `void setData(const QVector<double> &keys, const QVector<double> &values)` | Replace the series data. Extra points beyond the shorter vector are ignored. |
| `void addData(double key, double value)` | Append a single point. |
| `void clearData()` | Remove all points. |
| `int dataCount() const` | Number of points. |
| `const QVector<double> &keys() const` | The x (key) values. |
| `const QVector<double> &values() const` | The y (value) values. |

### Appearance

| Method | Description |
|--------|-------------|
| `void setPen(const QPen &pen)` | Line/marker pen (colour and width). |
| `QPen pen() const` | Current pen. |
| `void setLineStyle(LineStyle style)` | Connecting-line style: solid `Line`, step variants, or `None` (markers only). Default `Line`. |
| `LineStyle lineStyle() const` | Current line style. |
| `void setScatterShape(ScatterShape shape, double size = 5.0)` | Draw a marker at each point. `Disc` is filled; the others are outlined. Default `NoScatter`. |
| `ScatterShape scatterShape() const` | Current scatter shape. |
| `void setName(const QString &name)` | Series name (stored for legends/labelling). |
| `QString name() const` | Current name. |

### Auto-scaling

| Method | Description |
|--------|-------------|
| `void rescaleKeyAxis(bool onlyEnlarge = false)` | Stretch the plot's `xAxis` to fit this series' keys. With `onlyEnlarge`, never shrinks the current range. |
| `void rescaleValueAxis(bool onlyEnlarge = false)` | Same for the `yAxis` against the values. |

---

## class `SimpleTarget`

The model behind a `Target` chart. Samples are `(x, y)` offsets from the centre
(e.g. RA drift on x, Dec drift on y). The newest sample is drawn brightest and
in red; older samples fade toward transparent, and a faint trail connects the
last few. Obtain it via `SimplePlot::target()`.

### Samples

| Method | Description |
|--------|-------------|
| `void addSample(double x, double y)` | Append one sample; honours the history limit (ring buffer). |
| `void setData(const QVector<double> &xs, const QVector<double> &ys)` | Replace the whole history at once. |
| `void clear()` | Remove all samples. |
| `int sampleCount() const` | Number of retained samples. |
| `double rms() const` | RMS radius (√mean(x²+y²)) of the retained samples — handy for a stats read-out. |

### History & scale

| Method | Description |
|--------|-------------|
| `void setHistorySize(int n)` | Max retained samples (`0` = unlimited). Default `100`. |
| `int historySize() const` | Current limit. |
| `void setRadius(double radius)` | Fixed displayed radius in data units; the outermost ring sits here (used when auto-scale is off). Default `4.0`. |
| `double radius() const` | Current radius. |
| `void setAutoScale(bool on)` | When on, the radius grows to keep all samples visible (rounded up to a nice multiple of the ring count). Default `false`. |
| `bool autoScale() const` | Auto-scale state. |

### Rings & appearance

| Method | Description |
|--------|-------------|
| `void setRingCount(int n)` | Number of concentric range rings. Default `4`. |
| `int ringCount() const` | Current ring count. |
| `void setRingPen(const QPen &pen)` | Pen for the rings. Default grey `(120,120,120)`. |
| `void setCrosshairPen(const QPen &pen)` | Pen for the centre crosshair and diagonals. Default grey `(150,150,150)`. |
| `void setPointColor(const QColor &c)` | Colour of the samples (older ones fade in alpha; the latest is always drawn red). Default gold `(255,215,0)`. |
| `void setTrailColor(const QColor &c)` | Colour of the history trail line connecting the most recent samples. Default semi-transparent gold `(255,215,0,90)`. |
| `QColor trailColor() const` | Current trail colour. |
| `void setLatestPointColor(const QColor &c)` | Colour of the most recent sample dot. Default red `(255,80,80)`. |
| `QColor latestPointColor() const` | Current latest-sample colour. |
| `void setPointSize(double s)` | Sample marker diameter in px (clamped to ≥0). Repaints. Default `3.0`. |
| `double pointSize() const` | Current marker diameter. |
| `void setLatestPointSize(double s)` | Diameter in px of the most recent (red) sample. Default `5.0`. |
| `double latestPointSize() const` | Current latest-sample diameter. |
| `void setTraceLength(int hops)` | Number of connecting hops drawn in the history trail (`1` = only the last segment, `0` = no trail). Clamped to ≥0. Default `1`. |
| `int traceLength() const` | Current trail length in hops. |
| `void setNonFadingFraction(double f)` | Fraction (0..1) of the newest points drawn at full opacity; the rest fade linearly from 25% (oldest) up to full. `0` = all fade, `1` = none fade. Clamped. Default `0.3`. |
| `double nonFadingFraction() const` | Current non-fading fraction. |
| `void setAxisLabels(const QString &horizontal, const QString &vertical)` | Captions at the axis ends. Defaults `"RA"` / `"Dec"`. |
| `void setUnit(const QString &unit)` | String appended to each ring's radius number (e.g. `"\""` for arcsec, `"'"` for arcmin, `" px"`). Empty by default. |
| `QString unit() const` | Current unit suffix. |

---

## struct `SimpleRange`

A lightweight `[lower, upper]` interval (cf. `QCPRange`).

| Member / Method | Description |
|-----------------|-------------|
| `double lower` | Lower bound (default `0.0`). |
| `double upper` | Upper bound (default `1.0`). |
| `SimpleRange()` | Default range `0..1`. |
| `SimpleRange(double l, double u)` | Range `l..u`. |
| `double size() const` | `upper - lower`. |
| `double center() const` | Midpoint. |
| `bool contains(double v) const` | Whether `v` lies in `[lower, upper]`. |

---

## Examples

### Line / strip chart (focuser/guider drift)

```cpp
auto *plot = new SimplePlot(SimplePlot::Graph);
plot->setBackground(QBrush(QColor(0, 0, 0, 0)));

plot->addGraph();
plot->graph(0)->setPen(QPen(Qt::red));                 // RA
plot->addGraph();
plot->graph(1)->setPen(QPen(QColor(3, 172, 240)));     // Dec

plot->xAxis2->setVisible(true);                        // draw top frame
plot->yAxis2->setVisible(true);                        // draw right frame
plot->xAxis->setAutoTickCount(4);
plot->yAxis->setAutoTickCount(5);
plot->yAxis->setRange(-6, 6);
plot->xAxis->setShowEndLabel(true);                    // label the last x value

// feed data
QVector<double> x, ra, dec;          // fill these ...
plot->graph(0)->setData(x, ra);
plot->graph(1)->setData(x, dec);
plot->xAxis->setRange(0, x.isEmpty() ? 1 : x.size() - 0.8);
plot->replot();

// hide the x numbers (keep ticks/grid):
plot->xAxis->setTickLabels(false);
```

### Target scatter (guiding)

```cpp
auto *plot = new SimplePlot(SimplePlot::Target);
plot->setBackground(QBrush(QColor(0, 0, 0, 0)));

auto *t = plot->target();
t->setRadius(6.0);                 // 6-unit outer ring
t->setRingCount(3);                // rings at 2 / 4 / 6
t->setHistorySize(120);
t->setAxisLabels("RA", "Dec");
t->setUnit("\"");                  // arcsec suffix on ring numbers
t->setPointColor(QColor(40, 200, 60));   // green dots, latest auto-red

// per guide frame:
t->addSample(ra_drift, dec_drift);
double err = t->rms();             // e.g. for an "RMS: x.xx" label
```

### Porting an existing `FocusGraph` (QCustomPlot)

```cpp
// before:  class FocusGraph : public QCustomPlot
// after:   class FocusGraph : public SimplePlot   // construct with Graph
```

The body is unchanged except that the two
`connect(xAxis, SIGNAL(rangeChanged(QCPRange)), xAxis2, SLOT(setRange(QCPRange)))`
lines can be removed — `xAxis2`/`yAxis2` mirror the primary ranges
automatically.
