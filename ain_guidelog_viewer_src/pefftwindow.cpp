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
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "pecurve.h"
#include "verticallabel.h"

#include <simpleplot.h>

#include <algorithm>

namespace {
constexpr double kMaxPeriodS = 1600.0; // hard cap for the period axis, regardless of the data
}

PEFFTWindow::PEFFTWindow(QWidget *parent)
    : QMainWindow(parent, Qt::Window) {
	setWindowTitle(tr("RA Periodic Error Spectrum"));
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

void PEFFTWindow::createUi() {
	QWidget *central = new QWidget(this);
	setCentralWidget(central);

	QVBoxLayout *rootLayout = new QVBoxLayout(central);
	rootLayout->setContentsMargins(6, 6, 6, 6);
	rootLayout->setSpacing(6);

	m_summaryLabel = new QLabel("Load a session to compute the RA periodic error spectrum.", central);
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
	rootLayout->addWidget(m_summaryLabel);

	// --- Plot ---
	m_plot = new SimplePlot(SimplePlot::Graph, central);
	m_plot->setPlotMargins(56, 12, 16, 28);
	m_plot->xAxis->setLabel("Period (s)");
	m_plot->yAxis->setLabel("Relative amplitude");
	m_plot->xAxis2->setVisible(true);
	m_plot->yAxis2->setVisible(true);
	m_plot->xAxis2->setTickLabels(false);
	m_plot->yAxis2->setTickLabels(false);
	m_plot->installEventFilter(this); // reposition the floating peak labels on resize

	// SimplePlot's Graph mode does not paint axis captions, so draw them as
	// separate widgets: a rotated label to the left of the plot for Y, and a
	// centered label beneath it for X.
	m_yCaptionLabel = new VerticalLabel(central);
	m_yCaptionLabel->setText("Relative amplitude");

	QHBoxLayout *plotRow = new QHBoxLayout();
	plotRow->setContentsMargins(0, 0, 0, 0);
	plotRow->setSpacing(2);
	plotRow->addWidget(m_yCaptionLabel);
	plotRow->addWidget(m_plot, 1);
	rootLayout->addLayout(plotRow, 1);

	m_xCaptionLabel = new QLabel("Period (s)", central);
	m_xCaptionLabel->setAlignment(Qt::AlignHCenter);
	rootLayout->addWidget(m_xCaptionLabel);

	// Keep both captions visually identical (the rotated one otherwise inherits
	// a different effective font than the styled QLabel).
	m_yCaptionLabel->setFont(m_xCaptionLabel->font());
}

void PEFFTWindow::setSession(const QStringList &headers,
                             const QVector<QStringList> &rows,
                             double calibrationPxPerS,
                             double mountDecDeg) {
	m_headers = headers;
	m_rows = rows;
	m_calibrationPxPerS = calibrationPxPerS;
	m_mountDecDeg = mountDecDeg;
	recompute();
}

void PEFFTWindow::updateRows(const QStringList &headers,
                             const QVector<QStringList> &rows) {
	m_headers = headers;
	m_rows = rows;
	recompute();
}

void PEFFTWindow::recompute() {
	if (!m_plot) {
		return;
	}
	m_plot->clearGraphs();
	m_plot->clearCustomXAxisTicks();
	m_peaks.clear();
	layoutPeakLabels(); // drop any labels left over from a previous session

	PECurveOptions options;
	options.ratePxPerS = m_calibrationPxPerS;
	options.decDeg = m_mountDecDeg;
	options.arcsec = true;         // the unit cancels out once the spectrum is normalized
	options.removeDrift = true;    // always detrend so the PE isn't swamped by drift
	options.linearDetrend = false; // use the periodic-error-aware fit

	const PECurveData data = PECurve::reconstruct(m_headers, m_rows, options);
	if (!data.valid) {
		m_summaryLabel->setText(data.message);
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(-1, 1);
		m_plot->replot();
		return;
	}
	if (!data.hasRate) {
		m_summaryLabel->setText("This session's log has no RA calibration, so the periodic error "
		                        "can't be reconstructed for its spectrum.");
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(-1, 1);
		m_plot->replot();
		return;
	}

	const PEFFTData fft = PECurve::computeFFT(data.x, data.pe);
	if (!fft.valid) {
		m_summaryLabel->setText(fft.message);
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(-1, 1);
		m_plot->replot();
		return;
	}

	// Frequency/period unit differs when the log carried no timestamps: the FFT
	// still runs (over the sample index), but "Hz"/"s" would be meaningless.
	const QString freqUnit = data.usedTime ? QStringLiteral("Hz") : QStringLiteral("cycles/sample");
	const QString periodUnit = data.usedTime ? QStringLiteral("s") : QStringLiteral("samples");
	const QString periodAxisLabel = data.usedTime ? QStringLiteral("Period (s)")
	                                              : QStringLiteral("Period (samples)");
	m_xCaptionLabel->setText(periodAxisLabel);

	const QVector<PEFFTPeak> peaks = PECurve::findHarmonics(fft, 0.4, kMaxPeriodS);
	if (peaks.isEmpty()) {
		m_summaryLabel->setText("No dominant periodic component found in this session.");
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(0, 1);
		m_plot->replot();
		return;
	}

	auto num = [](double v, int prec) { return QString::number(v, 'f', prec); };

	// Plot period (seconds) instead of frequency, normalized so the
	// fundamental's amplitude is 1.0: DC (infinite period) is skipped, and bins
	// are walked from the Nyquist end down so period increases left to right.
	const double normBy = peaks.first().amplitude;
	QVector<double> periodX;
	QVector<double> ampY;
	periodX.reserve(fft.freq.size() - 1);
	ampY.reserve(fft.freq.size() - 1);
	for (int k = fft.freq.size() - 1; k >= 1; k--) {
		periodX.append(1.0 / fft.freq.at(k));
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

	// Zoom to the interesting period region (from a bit past the highest
	// harmonic's period down to a bit past the longest detected peak's
	// period), rather than the huge, near-empty tail near DC. Cap the upper
	// end so a single very long period can't stretch the axis out to where
	// the rest of the spectrum is squeezed into a sliver.
	double maxHarmonicFreq = 0.0;
	double maxPeakPeriod = 0.0;
	for (const PEFFTPeak &p : peaks) {
		if (p.periodS < kMaxPeriodS) {
			maxHarmonicFreq = std::max(maxHarmonicFreq, p.frequencyHz);
		}
		maxPeakPeriod = std::max(maxPeakPeriod, p.periodS);
	}
	const double xLower = 1.0 / (maxHarmonicFreq * 1.4);
	const double xUpper = std::min(maxPeakPeriod * 1.3, kMaxPeriodS);
	m_plot->xAxis->setRange(xLower, xUpper);
	m_plot->yAxis->setRange(0.0, 1.15);

	// Use the plot's regular evenly-spaced numeric ticks (rather than custom
	// labels only at the peaks) so every tick on the axis carries a number;
	// the fundamental/harmonics are labelled next to their points instead.
	m_plot->clearCustomXAxisTicks();
	m_plot->replot();

	m_peaks = peaks;
	m_peakPeriodUnit = periodUnit;
	layoutPeakLabels();

	const QString sep = QStringLiteral(" &nbsp;&nbsp;&middot;&nbsp;&nbsp; ");
	auto twoLines = [](const QString &a, const QString &b) {
		return QStringLiteral("<div style='margin:0'>") + a +
		       QStringLiteral("</div><div style='margin-top:7px'>") + b +
		       QStringLiteral("</div>");
	};

	const PEFFTPeak &fundamental = peaks.first();
	const QString line1 = "<b>Fundamental:</b>&nbsp;&nbsp; Period <b>" + num(fundamental.periodS, 1) + "</b> " + periodUnit +
	                      sep + "Frequency " + num(fundamental.frequencyHz, 5) + " " + freqUnit;

	QStringList harmonicParts;
	for (int i = 1; i < peaks.size(); i++) {
		const PEFFTPeak &p = peaks.at(i);
		harmonicParts << QString("<b>%1&times;f\u2080</b> (%2 %3, %4% amplitude)")
		                     .arg(p.harmonic)
		                     .arg(num(p.periodS, 1))
		                     .arg(periodUnit)
		                     .arg(num(p.relativeAmplitude * 100.0, 0));
	}
	const QString line2 = harmonicParts.isEmpty()
	                           ? "<b>Harmonics:</b>&nbsp;&nbsp; none reach 40% of the fundamental's amplitude."
	                           : "<b>Harmonics &ge;40% amplitude:</b>&nbsp;&nbsp; " + harmonicParts.join(sep);
	m_summaryLabel->setText(twoLines(line1, line2));
}

void PEFFTWindow::layoutPeakLabels() {
	qDeleteAll(m_peakLabels);
	m_peakLabels.clear();
	if (!m_plot || m_peaks.isEmpty()) {
		return;
	}

	// SimplePlot has no text-annotation API of its own (only axis-tick text),
	// so the labels are plain QLabel children positioned in plot-pixel space
	// using the same left/top-anchored mapping the widget paints with.
	const QRect area = m_plot->rect().adjusted(m_plot->marginLeft(), m_plot->marginTop(),
	                                           -m_plot->marginRight(), -m_plot->marginBottom());
	const SimpleRange xr = m_plot->xAxis->range();
	const SimpleRange yr = m_plot->yAxis->range();
	if (xr.size() <= 0.0 || yr.size() <= 0.0 || area.width() <= 0 || area.height() <= 0) {
		return;
	}

	for (const PEFFTPeak &p : m_peaks) {
		if (p.periodS < xr.lower || p.periodS > xr.upper) {
			continue; // outside the (3600s-capped) visible range
		}
		const double px = area.left() + (p.periodS - xr.lower) / xr.size() * area.width();
		const double py = area.bottom() - (p.relativeAmplitude - yr.lower) / yr.size() * area.height();

		const QString designation = p.harmonic == 1 ? QStringLiteral("f\u2080") : QString("%1f\u2080").arg(p.harmonic);
		const QString text = designation + " " + QString::number(p.periodS, 'f', 0) + m_peakPeriodUnit;

		QLabel *label = new QLabel(text, m_plot);
		label->setStyleSheet("color: rgb(255, 210, 90); background: transparent;");
		QFont f = label->font();
		f.setPointSizeF(qMax(7.0, f.pointSizeF() - 1.0));
		label->setFont(f);
		label->adjustSize();

		int lx = static_cast<int>(px) + 6;
		int ly = static_cast<int>(py) - label->height() - 2;
		lx = qBound(area.left(), lx, qMax(area.left(), area.right() - label->width()));
		ly = qBound(area.top(), ly, qMax(area.top(), area.bottom() - label->height()));
		label->move(lx, ly);
		label->show();
		m_peakLabels.append(label);
	}
}

bool PEFFTWindow::eventFilter(QObject *obj, QEvent *event) {
	if (obj == m_plot && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
		layoutPeakLabels();
	}
	return QMainWindow::eventFilter(obj, event);
}
