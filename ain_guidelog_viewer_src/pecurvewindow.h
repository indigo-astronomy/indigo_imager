// Copyright (c) 2026
// All rights reserved.

#ifndef PECURVEWINDOW_H
#define PECURVEWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QVector>

class QLabel;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class SimplePlot;
class VerticalLabel;

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
class PECurveWindow : public QMainWindow {
public:
	explicit PECurveWindow(QWidget *parent = nullptr);

	// Feeds a new session into the window and pre-fills the calibration from the
	// log (calibrationPxPerS <= 0 means the log carried none; the user can enter
	// one by hand). Use this on open / session change.
	void setSession(const QStringList &headers,
	                const QVector<QStringList> &rows,
	                double calibrationPxPerS);

	// Replaces only the plotted rows (e.g. the graph's visible window changed),
	// leaving the user's calibration entry untouched.
	void updateRows(const QStringList &headers,
	                const QVector<QStringList> &rows);

private:
	void createUi();
	void recompute();

	QDoubleSpinBox *m_calibrationSpin;
	QDoubleSpinBox *m_decSpin;
	QComboBox *m_unitCombo;
	QCheckBox *m_smoothCheck;
	QCheckBox *m_smoothResidualCheck;
	QCheckBox *m_detrendCheck;
	QCheckBox *m_linearDetrendCheck;
	QLabel *m_summaryLabel;
	QLabel *m_xCaptionLabel;
	VerticalLabel *m_yCaptionLabel;
	SimplePlot *m_plot;

	QStringList m_headers;
	QVector<QStringList> m_rows;
	double m_logCalibration = 0.0;
};

#endif // PECURVEWINDOW_H
