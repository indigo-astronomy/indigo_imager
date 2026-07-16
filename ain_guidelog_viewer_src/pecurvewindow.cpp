// Copyright (c) 2026
// All rights reserved.

#include "pecurvewindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QSignalBlocker>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "pecurve.h"

#include <simpleplot.h>

#include <algorithm>

// A thin widget that draws a single line of text rotated 90° (reading
// bottom-to-top), used for the vertical Y-axis caption. No signals/slots, so no
// Q_OBJECT / moc needed.
class VerticalLabel : public QWidget {
public:
	explicit VerticalLabel(QWidget *parent = nullptr) : QWidget(parent) {}

	void setText(const QString &text) {
		m_text = text;
		updateGeometry();
		update();
	}

	QSize sizeHint() const override {
		const QFontMetrics fm(font());
		return QSize(fm.height() + 4, fm.horizontalAdvance(m_text) + 12);
	}
	QSize minimumSizeHint() const override {
		return QSize(sizeHint().width(), 0);
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::TextAntialiasing, true);
		p.setPen(QColor(0xef, 0xf0, 0xf1)); // match the qdarkstyle caption text
		p.setFont(font());
		p.translate(0, height());
		p.rotate(-90.0);
		p.drawText(QRect(0, 0, height(), width()), Qt::AlignCenter, m_text);
	}

private:
	QString m_text;
};

PECurveWindow::PECurveWindow(QWidget *parent)
    : QMainWindow(parent, Qt::Window) {
	setWindowTitle(tr("Reconstructed RA Periodic Error"));
	resize(1000, 620);
	setWindowIcon(QIcon(":/resource/ain_guidelog_viewer.png"));

	QFile f(":/resource/control_panel.qss");
	if (f.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream ts(&f);
		setStyleSheet(ts.readAll());
		f.close();
	}

	createUi();
	recompute();
}

void PECurveWindow::createUi() {
	QWidget *central = new QWidget(this);
	setCentralWidget(central);

	QVBoxLayout *rootLayout = new QVBoxLayout(central);
	rootLayout->setContentsMargins(6, 6, 6, 6);
	rootLayout->setSpacing(6);

	// --- Controls row ---
	QHBoxLayout *controls = new QHBoxLayout();

	m_calibrationSpin = new QDoubleSpinBox(central);
	m_calibrationSpin->setDecimals(4);
	m_calibrationSpin->setRange(0.0, 100000.0);
	m_calibrationSpin->setSingleStep(0.1);
	m_calibrationSpin->setValue(0.0);
	m_calibrationSpin->setSpecialValueText("Not set");
	m_calibrationSpin->setFixedWidth(110);
	m_calibrationSpin->setToolTip("RA guide rate in pixels per second of guide pulse.\n"
	                              "Taken from the log's Calibration line when present.");

	m_decSpin = new QDoubleSpinBox(central);
	m_decSpin->setDecimals(1);
	m_decSpin->setRange(-89.0, 89.0);
	m_decSpin->setSingleStep(1.0);
	m_decSpin->setValue(0.0);
	m_decSpin->setSuffix("°");
	m_decSpin->setFixedWidth(80);
	m_decSpin->setToolTip("Target declination. The guider scales RA pulses by cos(dec),\n"
	                      "so this rescales the reconstruction to match. Not stored in\n"
	                      "the log — enter it by hand (0 = no scaling).");

	m_unitCombo = new QComboBox(central);
	m_unitCombo->addItem("arcsec", QStringLiteral("arcsec"));
	m_unitCombo->addItem("pixels", QStringLiteral("px"));
	m_unitCombo->setFixedWidth(90);

	m_smoothCheck = new QCheckBox("PE smoothing", central);
	m_smoothCheck->setToolTip("Show the periodic-error curve as a moving-average\n"
	                          "smoothed trace to reveal the underlying trend.");

	m_smoothResidualCheck = new QCheckBox("Residual smoothing", central);
	m_smoothResidualCheck->setToolTip("Show the residual-error curve as a moving-average\n"
	                                  "smoothed trace.");

	m_detrendCheck = new QCheckBox("PE deterend", central);
	m_detrendCheck->setChecked(true);
	m_detrendCheck->setToolTip("Subtract the linear drift trend (e.g. from polar-alignment\n"
	                           "error) so the periodic error is not swamped by a slope.");

	m_linearDetrendCheck = new QCheckBox("Linear detrend", central);
	m_linearDetrendCheck->setToolTip("Remove the drift with a plain straight-line fit instead of the\n"
	                                 "periodic-error-aware fit. A plain line can tilt a symmetric PE\n"
	                                 "when the window is not a whole number of worm periods; use this\n"
	                                 "only as a cross-check.");
	m_linearDetrendCheck->setEnabled(m_detrendCheck->isChecked());

	m_summaryLabel = new QLabel("Load a session to reconstruct the RA periodic error.", central);
	m_summaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_summaryLabel->setTextFormat(Qt::RichText);
	m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	{
		QFont summaryFont = m_summaryLabel->font();
		if (summaryFont.pointSizeF() > 0.0) {
			summaryFont.setPointSizeF(summaryFont.pointSizeF() + 2.0);
		} else {
			summaryFont.setPixelSize(summaryFont.pixelSize() + 2);
		}
		m_summaryLabel->setFont(summaryFont);
	}

	controls->addWidget(new QLabel("Calibration (px/s):"));
	controls->addWidget(m_calibrationSpin);
	controls->addSpacing(8);
	controls->addWidget(new QLabel("Dec:"));
	controls->addWidget(m_decSpin);
	controls->addSpacing(8);
	controls->addWidget(new QLabel("Units:"));
	controls->addWidget(m_unitCombo);
	controls->addSpacing(8);
	controls->addWidget(m_detrendCheck);
	controls->addSpacing(8);
	controls->addWidget(m_linearDetrendCheck);
	controls->addSpacing(8);
	controls->addWidget(m_smoothCheck);
	controls->addSpacing(8);
	controls->addWidget(m_smoothResidualCheck);
	controls->addStretch(1);
	rootLayout->addLayout(controls);

	// Stats on their own line beneath the controls.
	rootLayout->addWidget(m_summaryLabel);

	// --- Plot ---
	m_plot = new SimplePlot(SimplePlot::Graph, central);
	m_plot->setPlotMargins(56, 12, 16, 28);
	m_plot->xAxis->setLabel("Elapsed time (s)");
	m_plot->yAxis->setLabel("RA (arcsec)");
	m_plot->xAxis2->setVisible(true);
	m_plot->yAxis2->setVisible(true);
	m_plot->xAxis2->setTickLabels(false);
	m_plot->yAxis2->setTickLabels(false);

	// SimplePlot's Graph mode does not paint axis captions, so draw them as
	// separate widgets: a rotated label to the left of the plot for Y, and a
	// centered label beneath it for X.
	m_yCaptionLabel = new VerticalLabel(central);
	m_yCaptionLabel->setText("RA (arcsec)");

	QHBoxLayout *plotRow = new QHBoxLayout();
	plotRow->setContentsMargins(0, 0, 0, 0);
	plotRow->setSpacing(2);
	plotRow->addWidget(m_yCaptionLabel);
	plotRow->addWidget(m_plot, 1);
	rootLayout->addLayout(plotRow, 1);

	m_xCaptionLabel = new QLabel("Elapsed time (s)", central);
	m_xCaptionLabel->setAlignment(Qt::AlignHCenter);
	rootLayout->addWidget(m_xCaptionLabel);

	// Keep both captions visually identical (the rotated one otherwise inherits
	// a different effective font than the styled QLabel).
	m_yCaptionLabel->setFont(m_xCaptionLabel->font());

	connect(m_calibrationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, [this](double) { recompute(); });
	connect(m_decSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, [this](double) { recompute(); });
	connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, [this](int) { recompute(); });
	connect(m_smoothCheck, &QCheckBox::toggled, this, [this](bool) { recompute(); });
	connect(m_smoothResidualCheck, &QCheckBox::toggled, this, [this](bool) { recompute(); });
	connect(m_detrendCheck, &QCheckBox::toggled, this, [this](bool checked) {
		m_linearDetrendCheck->setEnabled(checked); // linear detrend only applies when removing drift
		recompute();
	});
	connect(m_linearDetrendCheck, &QCheckBox::toggled, this, [this](bool) { recompute(); });
}

void PECurveWindow::setSession(const QStringList &headers,
                               const QVector<QStringList> &rows,
                               double calibrationPxPerS) {
	m_headers = headers;
	m_rows = rows;
	m_logCalibration = calibrationPxPerS;

	// Pre-fill the calibration from the log when it carried one. The user can
	// still override it by hand afterwards.
	if (calibrationPxPerS > 0.0) {
		const QSignalBlocker blocker(m_calibrationSpin);
		m_calibrationSpin->setValue(calibrationPxPerS);
	}

	recompute();
}

void PECurveWindow::updateRows(const QStringList &headers,
                               const QVector<QStringList> &rows) {
	m_headers = headers;
	m_rows = rows;
	recompute();
}

void PECurveWindow::recompute() {
	if (!m_plot) {
		return;
	}
	m_plot->clearGraphs();
	m_plot->clearCustomXAxisTicks();

	const bool arcsecUnit = (m_unitCombo->currentData().toString() == "arcsec");
	const QString unitLabel = arcsecUnit ? QStringLiteral("arcsec") : QStringLiteral("px");
	m_yCaptionLabel->setText(QString("RA (%1)").arg(unitLabel));

	PECurveOptions options;
	options.ratePxPerS = m_calibrationSpin->value();
	options.decDeg = m_decSpin->value();
	options.arcsec = arcsecUnit;
	options.removeDrift = m_detrendCheck->isChecked();
	options.linearDetrend = m_linearDetrendCheck->isChecked();

	const PECurveData data = PECurve::reconstruct(m_headers, m_rows, options);
	if (!data.valid) {
		m_summaryLabel->setText(data.message);
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(-1, 1);
		m_plot->replot();
		return;
	}

	// The smoothing toggles only change what is drawn (moving-average traces);
	// the reported numbers below are unaffected.
	const bool smoothPe = m_smoothCheck->isChecked();
	const bool smoothRes = m_smoothResidualCheck->isChecked();
	const QVector<double> peSeries =
		smoothPe ? PECurve::smooth(data.pe, PECurve::autoSmoothWindow(data.pe.size())) : data.pe;
	const QVector<double> resSeries =
		smoothRes ? PECurve::smooth(data.residual, PECurve::autoSmoothWindow(data.residual.size())) : data.residual;

	m_xCaptionLabel->setText(data.usedTime ? "Elapsed time (s)" : "Sample index");

	SimpleGraph *gResidual = m_plot->addGraph();
	gResidual->setPen(QPen(QColor(120, 120, 120)));
	gResidual->setData(data.x, resSeries);
	gResidual->setName(smoothRes ? "Residual (smoothed)" : "Residual");

	SimpleGraph *gPe = m_plot->addGraph();
	QPen pePen(QColor(255, 190, 40));
	pePen.setWidth(2);
	gPe->setPen(pePen);
	gPe->setData(data.x, peSeries);
	gPe->setName(smoothPe ? "Periodic error (smoothed)" : "Periodic error");

	double xLower = data.x.first();
	double xUpper = data.x.last();
	if (xUpper <= xLower) {
		xLower -= 0.5;
		xUpper += 0.5;
	}
	m_plot->xAxis->setRange(xLower, xUpper);

	// Fit the vertical range to whatever is actually drawn (residual + PE).
	const double yLo = std::min(*std::min_element(resSeries.begin(), resSeries.end()),
	                            *std::min_element(peSeries.begin(), peSeries.end()));
	const double yHi = std::max(*std::max_element(resSeries.begin(), resSeries.end()),
	                            *std::max_element(peSeries.begin(), peSeries.end()));
	const double span = yHi - yLo;
	const double pad = (span > 0.0) ? span * 0.1 : 1.0;
	m_plot->yAxis->setRange(yLo - pad, yHi + pad);
	m_plot->replot();

	// Peak-to-peak follows the displayed PE trace (smoothed p-p is meaningful;
	// raw p-p is just noise spikes), but RMS stays on the raw curve so it does
	// not drift as the smoothing box is toggled.
	const double peP2P = PECurve::peakToPeak(peSeries);
	const double peRms = PECurve::rms(data.pe);

	auto num = [](double v, int prec) { return QString::number(v, 'f', prec); };
	const QString &u = unitLabel;
	const QString sep = QStringLiteral(" &nbsp;&nbsp;&middot;&nbsp;&nbsp; ");
	// Render the two stat lines as separate blocks so we control the gap between
	// them (a plain <br> is too tight).
	auto twoLines = [](const QString &a, const QString &b) {
		return QStringLiteral("<div style='margin:0'>") + a +
		       QStringLiteral("</div><div style='margin-top:7px'>") + b +
		       QStringLiteral("</div>");
	};

	if (!data.hasRate) {
		const QString line1 = "<b>Residual</b>&nbsp;&nbsp; peak-to-peak <b>" + num(peP2P, 3) + "</b> " + u +
		                      sep + "RMS <b>" + num(peRms, 3) + "</b> " + u;
		const QString line2 = "Enter the RA calibration (px/s) to reconstruct the periodic error "
		                      "and its suppression.";
		m_summaryLabel->setText(twoLines(line1, line2));
		return;
	}

	// Two suppression figures, both as an RMS ratio of residual to reconstructed
	// PE. "Total error suppression" uses the raw curves, so it includes the
	// atmospheric seeing / centroid noise the loop cannot remove. "Periodic
	// error suppression" uses the smoothed curves, removing that high-frequency
	// content to isolate how well the slow periodic error itself was corrected.
	const int window = PECurve::autoSmoothWindow(data.pe.size());
	const QVector<double> peSmooth = PECurve::smooth(data.pe, window);
	const QVector<double> resSmooth = PECurve::smooth(data.residual, window);
	const double peRmsRaw = PECurve::rms(data.pe);
	const double resRmsRaw = PECurve::rms(data.residual);
	const double peRmsSm = PECurve::rms(peSmooth);
	const double resRmsSm = PECurve::rms(resSmooth);
	const double totalSuppr = (peRmsRaw > 0.0) ? (1.0 - resRmsRaw / peRmsRaw) * 100.0 : 0.0;
	const double periodicSuppr = (peRmsSm > 0.0) ? (1.0 - resRmsSm / peRmsSm) * 100.0 : 0.0;

	// Suppression factor: how many times smaller the residual is than the PE.
	auto factorStr = [](double peRms, double resRms) -> QString {
		if (peRms <= 0.0) return QStringLiteral("0");
		if (resRms <= 0.0) return QStringLiteral("&infin;");
		return QString::number(peRms / resRms, 'f', 1);
	};

	const QString line1 = "<b>Periodic error:</b>&nbsp;&nbsp; Peak-to-Peak <b>" + num(peP2P, 3) + "</b> " + u +
	                      sep + "RMS <b>" + num(peRms, 3) + "</b> " + u +
	                      sep + "Residual RMS <b>" + num(resRmsRaw, 3) + "</b> " + u;
	const QString line2 = "<b>Suppression:</b>&nbsp;&nbsp; Periodic error <b>" + num(periodicSuppr, 1) + "%</b> (" +
	                      factorStr(peRmsSm, resRmsSm) + "&times;)" +
	                      sep + "Overall <b>" + num(totalSuppr, 1) + "%</b> (" +
	                      factorStr(peRmsRaw, resRmsRaw) + "&times;)";
	m_summaryLabel->setText(twoLines(line1, line2));
}
