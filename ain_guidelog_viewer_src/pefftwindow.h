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

#ifndef PEFFTWINDOW_H
#define PEFFTWINDOW_H

#include <QString>
#include <QVector>

#include "peanalysis.h"
#include "pecurve.h"
#include "pewindowbase.h"

class QEvent;
class QLabel;
class QPushButton;
class QStandardItemModel;
class QTableView;

// Shows the amplitude spectrum (FFT) of the reconstructed RA periodic-error
// curve (same reconstruction as PECurveWindow, always with the drift trend
// removed). The fundamental is labelled on the plot; it and every harmonic
// above the amplitude threshold are listed in the table beside it.
//
// The calibration and declination come from the log and are used as-is; there
// is no manual override in this window (see PECurveWindow for that).
class PEFFTWindow : public PEWindowBase {
	Q_OBJECT

public:
	explicit PEFFTWindow(QWidget *parent = nullptr);

protected:
	void recompute() override;
	void applyLogDefaults(double calibrationPxPerS, double mountDecDeg) override;
	bool eventFilter(QObject *obj, QEvent *event) override;

private:
	// Builds the harmonics table shown to the right of the plot.
	void createPeakTable();
	// Fills that table from m_peaks, with amplitudes as a percentage of
	// normalizeBy (the strongest peak, so the numbers match the plot's y axis).
	// Clears it when there are no peaks.
	void fillPeakTable(double normalizeBy);
	// Positions one floating label per peak next to its marker on the plot, and
	// redraws them on resize (SimplePlot has no annotation API of its own, only
	// axis-tick text). Labels are reused across calls rather than recreated.
	void layoutPeakLabels();
	void exportCsv();

	double m_calibrationPxPerS = 0.0;
	double m_mountDecDeg = 0.0;

	QPushButton *m_exportButton = nullptr;
	QTableView *m_peakTable = nullptr;
	QStandardItemModel *m_peakModel = nullptr;

	QVector<PEFFTPeak> m_peaks;
	QString m_peakPeriodUnit;
	QVector<QLabel *> m_peakLabels;

	// Snapshot of the last computed spectrum, kept for CSV export. The result is
	// shared with PEAnalysis's cache, so holding it costs a reference count.
	std::shared_ptr<const PEResult> m_lastResult;
	double m_lastNormalizeBy = 0.0;
	bool m_lastUsedTime = false;
	QString m_lastAmplitudeUnit;
};

#endif // PEFFTWINDOW_H
