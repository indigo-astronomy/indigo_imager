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

#include "pecurvewindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>

#include "peanalysis.h"
#include "pecurve.h"
#include "verticallabel.h"

#include <simpleplot.h>

#include <algorithm>

PECurveWindow::PECurveWindow(QWidget *parent)
    : PEWindowBase(tr("Reconstructed RA Periodic Error"), parent) {
	createControls();
	addSummaryRow(m_exportButton);
	addPlotRow();

	m_plot->xAxis->setLabel("Elapsed time (s)");
	m_plot->yAxis->setLabel("RA (arcsec)");
	m_yCaptionLabel->setText("RA (arcsec)");
	m_xCaptionLabel->setText("Elapsed time (s)");

	m_summaryLabel->setText("Load a session to reconstruct the RA periodic error.");

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
	connect(m_exportButton, &QPushButton::clicked, this, [this]() { exportCsv(); });
}

void PECurveWindow::createControls() {
	QWidget *central = centralPanel();

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
	                      "so this rescales the reconstruction to match. Pre-filled from\n"
	                      "the log's Mount Coordinates line when present; otherwise enter\n"
	                      "it by hand (0 = no scaling).");

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

	m_exportButton = new QPushButton("Export CSV...", central);
	m_exportButton->setToolTip("Save the currently plotted periodic-error and residual curves to a CSV file.");

	QHBoxLayout *controls = new QHBoxLayout();
	controls->addWidget(new QLabel("Calibration (px/s):", central));
	controls->addWidget(m_calibrationSpin);
	controls->addSpacing(8);
	controls->addWidget(new QLabel("Dec:", central));
	controls->addWidget(m_decSpin);
	controls->addSpacing(8);
	controls->addWidget(new QLabel("Units:", central));
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
	rootLayout()->addLayout(controls);
}

void PECurveWindow::applyLogDefaults(double calibrationPxPerS, double mountDecDeg) {
	// Pre-fill the calibration from the log when it carried one. The user can
	// still override it by hand afterwards.
	if (calibrationPxPerS > 0.0) {
		const QSignalBlocker blocker(m_calibrationSpin);
		m_calibrationSpin->setValue(calibrationPxPerS);
	}

	// Same for the Dec spin box: pre-fill it from the log's Mount Coordinates
	// line when present, but leave the user's hand-entered value alone otherwise.
	if (mountDecDeg != 0.0) {
		const QSignalBlocker blocker(m_decSpin);
		m_decSpin->setValue(mountDecDeg);
	}
}

void PECurveWindow::recompute() {
	if (!m_plot) {
		return;
	}

	const bool arcsecUnit = (m_unitCombo->currentData().toString() == "arcsec");
	const QString unitLabel = arcsecUnit ? QStringLiteral("arcsec") : QStringLiteral("px");
	m_yCaptionLabel->setText(QString("RA (%1)").arg(unitLabel));

	if (!hasAnalysis()) {
		m_lastValid = false;
		showPlaceholder("Load a session to reconstruct the RA periodic error.");
		return;
	}

	PECurveOptions options;
	options.ratePxPerS = m_calibrationSpin->value();
	options.decDeg = m_decSpin->value();
	options.arcsec = arcsecUnit;
	options.removeDrift = m_detrendCheck->isChecked();
	options.linearDetrend = m_linearDetrendCheck->isChecked();

	const std::shared_ptr<const PEResult> result = analysis()->reconstruct(options);
	const PECurveData &data = result->curve;
	if (!data.valid) {
		m_lastValid = false;
		showPlaceholder(data.message);
		return;
	}

	m_plot->clearGraphs();
	m_plot->clearCustomXAxisTicks();

	// The smoothing toggles only change what is drawn (moving-average traces);
	// the reported numbers below are unaffected.
	const bool smoothPe = m_smoothCheck->isChecked();
	const bool smoothRes = m_smoothResidualCheck->isChecked();
	const QVector<double> peSeries =
		smoothPe ? PECurve::smooth(data.pe, PECurve::autoSmoothWindow(data.pe.size())) : data.pe;
	const QVector<double> resSeries =
		smoothRes ? PECurve::smooth(data.residual, PECurve::autoSmoothWindow(data.residual.size())) : data.residual;

	m_xCaptionLabel->setText(data.usedTime ? "Elapsed time (s)" : "Sample index");

	// Snapshot of what's actually plotted, for CSV export.
	m_lastValid = true;
	m_lastUsedTime = data.usedTime;
	m_lastXLabel = data.usedTime ? QStringLiteral("Elapsed time (s)") : QStringLiteral("Sample index");
	m_lastUnitLabel = unitLabel;
	m_lastX = data.x;
	m_lastPeSeries = peSeries;
	m_lastResSeries = resSeries;

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

	const QString &u = unitLabel;
	const QString sep = separator();

	if (!data.hasRate) {
		const QString line1 = "<b>Residual</b>&nbsp;&nbsp; peak-to-peak <b>" + number(peP2P, 3) + "</b> " + u +
		                      sep + "RMS <b>" + number(peRms, 3) + "</b> " + u;
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
	auto factorStr = [](double pe, double res) -> QString {
		if (pe <= 0.0) return QStringLiteral("0");
		if (res <= 0.0) return QStringLiteral("&infin;");
		return QString::number(pe / res, 'f', 1);
	};

	const QString line1 = "<b>Periodic error:</b>&nbsp;&nbsp; Peak-to-Peak <b>" + number(peP2P, 3) + "</b> " + u +
	                      sep + "RMS <b>" + number(peRms, 3) + "</b> " + u +
	                      sep + "Residual RMS <b>" + number(resRmsRaw, 3) + "</b> " + u;
	const QString line2 = "<b>Suppression:</b>&nbsp;&nbsp; Periodic error <b>" + number(periodicSuppr, 1) + "%</b> (" +
	                      factorStr(peRmsSm, resRmsSm) + "&times;)" +
	                      sep + "Overall <b>" + number(totalSuppr, 1) + "%</b> (" +
	                      factorStr(peRmsRaw, resRmsRaw) + "&times;)";
	m_summaryLabel->setText(twoLines(line1, line2));
}

void PECurveWindow::exportCsv() {
	if (!m_lastValid || m_lastX.isEmpty()) {
		QMessageBox::warning(this, tr("Export CSV"), tr("There is no reconstructed PE curve to export yet."));
		return;
	}

	const QString fileName = askForCsvPath(tr("Export PE curve as CSV"));
	if (fileName.isEmpty()) {
		return;
	}

	QString errorMessage;
	if (!PECurve::saveCsv(fileName, m_lastX, m_lastResSeries, m_lastPeSeries,
	                      m_lastUsedTime, m_lastXLabel, m_lastUnitLabel, &errorMessage)) {
		QMessageBox::warning(this, tr("Export CSV"), errorMessage);
	}
}
