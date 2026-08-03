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

#ifndef PECURVEWINDOW_H
#define PECURVEWINDOW_H

#include <QString>
#include <QVector>

#include "pewindowbase.h"

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QPushButton;

// Reconstructs and plots the RA periodic error (PE) curve of a guiding session.
//
// During guiding the mount's periodic error is mostly cancelled by the guide
// pulses, so the raw drift is hidden in the residual. It is recovered by adding
// the residual error back to the cumulative corrections that were applied:
//
//     PE(n) = residual(n) + sign * sum_{k<=n} ( correction_seconds(k) * rate )
//
// where rate is the RA guide rate in px/s (from the log's Calibration line or
// entered by hand). The result is shown in arcsec or pixels.
class PECurveWindow : public PEWindowBase {
	Q_OBJECT

public:
	explicit PECurveWindow(QWidget *parent = nullptr);

protected:
	void recompute() override;
	// Pre-fills the calibration and Dec entries from the log, leaving a value
	// the user typed in by hand alone.
	void applyLogDefaults(double calibrationPxPerS, double mountDecDeg) override;

private:
	void createControls();
	void exportCsv();

	QDoubleSpinBox *m_calibrationSpin = nullptr;
	QDoubleSpinBox *m_decSpin = nullptr;
	QComboBox *m_unitCombo = nullptr;
	QCheckBox *m_smoothCheck = nullptr;
	QCheckBox *m_smoothResidualCheck = nullptr;
	QCheckBox *m_detrendCheck = nullptr;
	QCheckBox *m_linearDetrendCheck = nullptr;
	QPushButton *m_exportButton = nullptr;

	// Snapshot of the last plotted curve, kept for CSV export.
	bool m_lastValid = false;
	bool m_lastUsedTime = false;
	QString m_lastXLabel;
	QString m_lastUnitLabel;
	QVector<double> m_lastX;
	QVector<double> m_lastPeSeries;
	QVector<double> m_lastResSeries;
};

#endif // PECURVEWINDOW_H
