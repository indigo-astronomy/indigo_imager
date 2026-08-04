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

#include "pefftwindow.h"

#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QMessageBox>
#include <QPointF>
#include <QPushButton>
#include <QRect>

#include "peanalysis.h"
#include "pecurve.h"
#include "verticallabel.h"

#include <simpleplot.h>

#include <algorithm>
#include <cmath>

namespace {
constexpr double kMaxPeriodS = 1600.0;        // hard cap for the period axis, regardless of the data
constexpr double kMinRelativeAmplitude = 0.4; // harmonics quieter than this are not labelled
} // namespace

PEFFTWindow::PEFFTWindow(QWidget *parent)
    : PEWindowBase(tr("RA Periodic Error Spectrum"), parent) {
	m_exportButton = new QPushButton("Export CSV...", centralPanel());
	m_exportButton->setToolTip("Save the full amplitude spectrum to a CSV file.");
	connect(m_exportButton, &QPushButton::clicked, this, [this]() { exportCsv(); });

	addSummaryRow(m_exportButton);
	addPlotRow();

	m_plot->xAxis->setLabel("Period (s)");
	m_plot->yAxis->setLabel("Relative amplitude");
	m_plot->installEventFilter(this); // reposition the floating peak labels on resize

	m_yCaptionLabel->setText("Relative amplitude");
	m_xCaptionLabel->setText("Period (s)");

	m_summaryLabel->setText("Load a session to compute the RA periodic error spectrum.");
}

void PEFFTWindow::applyLogDefaults(double calibrationPxPerS, double mountDecDeg) {
	m_calibrationPxPerS = calibrationPxPerS;
	m_mountDecDeg = mountDecDeg;
}

void PEFFTWindow::recompute() {
	if (!m_plot) {
		return;
	}
	m_peaks.clear();
	layoutPeakLabels(); // drop any labels left over from a previous session
	m_lastResult.reset(); // nothing exportable until a spectrum is actually drawn

	if (!hasAnalysis()) {
		showPlaceholder("Load a session to compute the RA periodic error spectrum.");
		return;
	}

	PECurveOptions options;
	options.ratePxPerS = m_calibrationPxPerS;
	options.decDeg = m_mountDecDeg;
	options.arcsec = true;         // the unit cancels out once the spectrum is normalized
	options.removeDrift = true;    // always detrend so the PE isn't swamped by drift
	options.linearDetrend = false; // use the periodic-error-aware fit

	const std::shared_ptr<const PEResult> result = analysis()->reconstructWithSpectrum(options);
	const PECurveData &data = result->curve;
	if (!data.valid) {
		showPlaceholder(data.message);
		return;
	}
	if (!data.hasRate) {
		showPlaceholder("This session's log has no RA calibration, so the periodic error "
		                "can't be reconstructed for its spectrum.");
		return;
	}

	const PEFFTData &fft = result->spectrum;
	if (!fft.valid) {
		showPlaceholder(fft.message);
		return;
	}

	// Frequency/period unit differs when the log carried no timestamps: the FFT
	// still runs (over the sample index), but "Hz"/"s" would be meaningless.
	const QString freqUnit = data.usedTime ? QStringLiteral("Hz") : QStringLiteral("cycles/sample");
	const QString periodUnit = data.usedTime ? QStringLiteral("s") : QStringLiteral("samples");
	m_xCaptionLabel->setText(data.usedTime ? QStringLiteral("Period (s)")
	                                       : QStringLiteral("Period (samples)"));

	const QVector<PEFFTPeak> peaks = PECurve::findHarmonics(fft, kMinRelativeAmplitude, kMaxPeriodS);
	if (peaks.isEmpty()) {
		showPlaceholder("No dominant periodic component found in this session.");
		return;
	}

	m_plot->clearGraphs();
	m_plot->clearCustomXAxisTicks();

	// Zoom to the interesting period region (from a bit past the highest
	// harmonic's period down to a bit past the longest detected peak's period),
	// rather than the huge, near-empty tail near DC. Cap the upper end so a
	// single very long period can't stretch the axis out to where the rest of
	// the spectrum is squeezed into a sliver.
	double maxHarmonicFreq = 0.0;
	double maxPeakPeriod = 0.0;
	for (const PEFFTPeak &p : peaks) {
		if (p.periodS < kMaxPeriodS) {
			maxHarmonicFreq = std::max(maxHarmonicFreq, p.frequencyHz);
		}
		maxPeakPeriod = std::max(maxPeakPeriod, p.periodS);
	}
	if (maxHarmonicFreq <= 0.0) {
		// findHarmonics() keeps the fundamental within kMaxPeriodS, so this only
		// happens if a peak lands exactly on the cap; fall back to that peak.
		maxHarmonicFreq = peaks.first().frequencyHz;
	}
	const double xLower = (maxHarmonicFreq > 0.0) ? 1.0 / (maxHarmonicFreq * 1.4) : 1.0;
	const double xUpper = std::max(xLower * 1.1, std::min(maxPeakPeriod * 1.3, kMaxPeriodS));
	m_plot->xAxis->setRange(xLower, xUpper);
	m_plot->yAxis->setRange(0.0, 1.15);

	// Plot period (seconds) instead of frequency, normalized so the fundamental's
	// amplitude is 1.0. Bins are walked from the Nyquist end down so period
	// increases left to right, and only the bins that land inside the visible
	// period window (plus one either side, so the trace reaches both edges) are
	// handed over: the spectrum is zero-padded to several times the sample count,
	// and at these zoom levels well over 90% of its bins sit off-screen. Since
	// SimplePlot maps every point it is given, clipping here is what keeps a
	// repaint cheap.
	const double normBy = peaks.first().amplitude;
	const double df = (fft.freq.size() > 1) ? fft.freq.at(1) : 0.0;
	const int lastBin = fft.freq.size() - 1;
	int binLow = 1;
	int binHigh = lastBin;
	if (df > 0.0 && normBy > 0.0) {
		binLow = static_cast<int>(std::floor((1.0 / xUpper) / df)) - 1;
		binHigh = static_cast<int>(std::ceil((1.0 / xLower) / df)) + 1;
		binLow = std::max(1, std::min(binLow, lastBin));
		binHigh = std::max(binLow, std::min(binHigh, lastBin));
	}

	QVector<double> periodX;
	QVector<double> ampY;
	periodX.reserve(binHigh - binLow + 1);
	ampY.reserve(binHigh - binLow + 1);
	for (int k = binHigh; k >= binLow; k--) {
		const double f = fft.freq.at(k);
		if (f <= 0.0) {
			continue;
		}
		periodX.append(1.0 / f);
		ampY.append(fft.amplitude.at(k) / normBy);
	}

	SimpleGraph *gSpectrum = m_plot->addGraph();
	QPen spectrumPen(QColor(120, 190, 255));
	spectrumPen.setWidth(1);
	gSpectrum->setPen(spectrumPen);
	gSpectrum->setData(periodX, ampY);
	gSpectrum->setName("Amplitude spectrum");

	// Highlight the fundamental / harmonic bins found above: a dashed vertical
	// line from the axis up to each peak, plus the marker dots themselves.
	for (const PEFFTPeak &p : peaks) {
		SimpleGraph *gLine = m_plot->addGraph();
		QPen linePen(QColor(255, 190, 40, 150));
		linePen.setStyle(Qt::DashLine);
		linePen.setWidth(1);
		gLine->setPen(linePen);
		gLine->setData(QVector<double>{p.periodS, p.periodS}, QVector<double>{0.0, p.relativeAmplitude});
	}
	{
		QVector<double> peakPeriod;
		QVector<double> peakAmp;
		peakPeriod.reserve(peaks.size());
		peakAmp.reserve(peaks.size());
		for (const PEFFTPeak &p : peaks) {
			peakPeriod.append(p.periodS);
			peakAmp.append(p.relativeAmplitude);
		}
		SimpleGraph *gPeaks = m_plot->addGraph();
		gPeaks->setLineStyle(SimpleGraph::None);
		gPeaks->setScatterShape(SimpleGraph::Disc, 7.0);
		gPeaks->setPen(QPen(QColor(255, 190, 40)));
		gPeaks->setData(peakPeriod, peakAmp);
		gPeaks->setName("Fundamental / harmonics");
	}

	// Use the plot's regular evenly-spaced numeric ticks (rather than custom
	// labels only at the peaks) so every tick on the axis carries a number;
	// the fundamental/harmonics are labelled next to their points instead.
	m_plot->replot();

	m_peaks = peaks;
	m_peakPeriodUnit = periodUnit;
	layoutPeakLabels();

	// Snapshot for CSV export. The reconstruction is always run in arcsec, but
	// that only holds when the log paired an arcsec column with the pixel one;
	// without it the scale stays 1 and the amplitudes are really in pixels.
	m_lastResult = result;
	m_lastNormalizeBy = normBy;
	m_lastUsedTime = data.usedTime;
	m_lastAmplitudeUnit = analysis()->samples().hasArcsecScale ? QStringLiteral("arcsec")
	                                                           : QStringLiteral("px");

	const PEFFTPeak &fundamental = peaks.first();
	const QString line1 = "<b>Fundamental:</b>&nbsp;&nbsp; Period <b>" + number(fundamental.periodS, 1) +
	                      "</b> " + periodUnit + separator() + "Frequency " +
	                      number(fundamental.frequencyHz, 5) + " " + freqUnit;

	QStringList harmonicParts;
	for (int i = 1; i < peaks.size(); i++) {
		const PEFFTPeak &p = peaks.at(i);
		harmonicParts << QString("<b>%1&times;f₀</b> (%2 %3, %4% amplitude)")
		                     .arg(p.harmonic)
		                     .arg(number(p.periodS, 1))
		                     .arg(periodUnit)
		                     .arg(number(p.relativeAmplitude * 100.0, 0));
	}
	const QString line2 = harmonicParts.isEmpty()
	                          ? "<b>Harmonics:</b>&nbsp;&nbsp; none reach 40% of the fundamental's amplitude."
	                          : "<b>Harmonics &ge;40% amplitude:</b>&nbsp;&nbsp; " +
	                                harmonicParts.join(separator());
	m_summaryLabel->setText(twoLines(line1, line2));
}

void PEFFTWindow::layoutPeakLabels() {
	if (!m_plot) {
		return;
	}

	// Labels are kept and re-used rather than deleted and rebuilt: this runs on
	// every resize event, and re-creating widgets (each with its own stylesheet
	// to parse) mid-drag is needless churn.
	const QRect area = m_plot->plotArea();
	const SimpleRange xr = m_plot->xAxis->range();
	const SimpleRange yr = m_plot->yAxis->range();
	const bool plottable = xr.size() > 0.0 && yr.size() > 0.0 && area.width() > 0 && area.height() > 0;

	int used = 0;
	if (plottable) {
		for (const PEFFTPeak &p : m_peaks) {
			if (p.periodS < xr.lower || p.periodS > xr.upper) {
				continue; // outside the visible (kMaxPeriodS-capped) range
			}
			const QPointF pos = m_plot->mapToPixel(p.periodS, p.relativeAmplitude);

			const QString designation =
				p.harmonic == 1 ? QStringLiteral("f₀") : QString("%1f₀").arg(p.harmonic);
			const QString text = designation + " " + QString::number(p.periodS, 'f', 0) +
			                     (m_peakPeriodUnit == QStringLiteral("s") ? QString() : QStringLiteral(" ")) +
			                     m_peakPeriodUnit;

			QLabel *label = nullptr;
			if (used < m_peakLabels.size()) {
				label = m_peakLabels.at(used);
			} else {
				label = new QLabel(m_plot);
				label->setStyleSheet("color: rgb(255, 210, 90); background: transparent;");
				QFont f = label->font();
				f.setPointSizeF(qMax(7.0, f.pointSizeF() - 1.0));
				label->setFont(f);
				m_peakLabels.append(label);
			}
			if (label->text() != text) {
				label->setText(text);
			}
			label->adjustSize();

			int lx = static_cast<int>(pos.x()) + 6;
			int ly = static_cast<int>(pos.y()) - label->height() - 2;
			lx = qBound(area.left(), lx, qMax(area.left(), area.right() - label->width()));
			ly = qBound(area.top(), ly, qMax(area.top(), area.bottom() - label->height()));
			label->move(lx, ly);
			label->show();
			used++;
		}
	}
	for (int i = used; i < m_peakLabels.size(); i++) {
		m_peakLabels.at(i)->hide();
	}
}

void PEFFTWindow::exportCsv() {
	if (!m_lastResult || !m_lastResult->spectrum.valid) {
		QMessageBox::warning(this, tr("Export CSV"),
		                     tr("There is no periodic-error spectrum to export yet."));
		return;
	}

	const QString fileName = askForCsvPath(tr("Export PE spectrum as CSV"));
	if (fileName.isEmpty()) {
		return;
	}

	QString errorMessage;
	if (!PECurve::saveSpectrumCsv(fileName, m_lastResult->spectrum, m_lastNormalizeBy,
	                              m_lastUsedTime, m_lastAmplitudeUnit, &errorMessage)) {
		QMessageBox::warning(this, tr("Export CSV"), errorMessage);
	}
}

bool PEFFTWindow::eventFilter(QObject *obj, QEvent *event) {
	if (obj == m_plot && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
		layoutPeakLabels();
	}
	return PEWindowBase::eventFilter(obj, event);
}
