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

#include <QMainWindow>
#include <QStringList>
#include <QVector>

#include "pecurve.h"

class QEvent;
class QLabel;
class SimplePlot;
class VerticalLabel;

// Shows the amplitude spectrum (FFT) of the reconstructed RA periodic-error
// curve (same reconstruction as PECurveWindow, always with the drift trend
// removed), and labels the fundamental period together with any harmonics
// whose amplitude is at least 40% of the fundamental's.
class PEFFTWindow : public QMainWindow {
public:
	explicit PEFFTWindow(QWidget *parent = nullptr);

	// Feeds a new session into the window. calibrationPxPerS/mountDecDeg come
	// from the log (Calibration / Mount Coordinates lines) and are used as-is;
	// there is no manual override in this window (see PECurveWindow for that).
	// Use this on open / session change.
	void setSession(const QStringList &headers,
	                const QVector<QStringList> &rows,
	                double calibrationPxPerS,
	                double mountDecDeg = 0.0);

	// Replaces only the plotted rows (e.g. the graph's visible window changed).
	void updateRows(const QStringList &headers,
	                const QVector<QStringList> &rows);

private:
	void createUi();
	void recompute();
	// Positions one floating label per peak next to its marker on the plot,
	// and redraws them on resize (the plotting widget itself has no
	// annotation API, only axis-tick text).
	void layoutPeakLabels();
	bool eventFilter(QObject *obj, QEvent *event) override;

	QLabel *m_summaryLabel;
	QLabel *m_xCaptionLabel;
	VerticalLabel *m_yCaptionLabel;
	SimplePlot *m_plot;

	QStringList m_headers;
	QVector<QStringList> m_rows;
	double m_calibrationPxPerS = 0.0;
	double m_mountDecDeg = 0.0;

	QVector<PEFFTPeak> m_peaks;
	QString m_peakPeriodUnit;
	QVector<QLabel *> m_peakLabels;
};

#endif // PEFFTWINDOW_H
