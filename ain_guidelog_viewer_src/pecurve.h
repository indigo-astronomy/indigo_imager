// Copyright (c) 2026
// All rights reserved.

#ifndef PECURVE_H
#define PECURVE_H

#include <QString>
#include <QStringList>
#include <QVector>

// Parameters controlling the RA periodic-error reconstruction.
struct PECurveOptions {
	double ratePxPerS = 0.0;   // RA guide rate (pixels per second of guide pulse)
	bool invert = false;       // flip the applied-correction sign convention
	bool arcsec = true;        // output unit: arcsec (true) or pixels (false)
	bool excludeDither = true; // drop dithering rows and interpolate the gaps
	bool removeDrift = false;  // subtract the linear drift trend (isolate the PE)
};

// Result of a reconstruction. Series are in the requested unit; x is elapsed
// seconds when usedTime is true, otherwise the sample index.
struct PECurveData {
	bool valid = false;
	QString message;      // human-readable reason when !valid
	bool usedTime = false;
	bool hasRate = false; // false when no calibration (px/s) was supplied
	QVector<double> x;
	QVector<double> residual;
	QVector<double> pe;
};

// UI-independent RA periodic-error math. Keeps all reconstruction / smoothing
// out of the window code (cf. GuideLogStats).
//
// The mount's periodic error is mostly cancelled by the guide pulses, so it is
// recovered by adding the residual back to the cumulative corrections:
//
//     PE(n) = residual(n) + sign * sum_{k<=n} ( correction_seconds(k) * rate )
class PECurve {
public:
	static PECurveData reconstruct(const QStringList &headers,
	                               const QVector<QStringList> &rows,
	                               const PECurveOptions &options);

	// Centered moving average; the window shrinks towards the ends so every
	// point stays defined. Returns the input unchanged for tiny sets / window<3.
	static QVector<double> smooth(const QVector<double> &data, int window);

	// A sensible odd smoothing window (~2% of the samples each side, min 3).
	static int autoSmoothWindow(int sampleCount);

	// Subtracts the least-squares straight-line fit of y over x (removes the
	// linear drift trend and DC offset). Returns the input unchanged if it can't
	// fit (fewer than two points, mismatched sizes, or all x equal).
	static QVector<double> detrend(const QVector<double> &x, const QVector<double> &y);

	// Peak-to-peak (max-min) and RMS of a series; 0 for empty input.
	static double peakToPeak(const QVector<double> &data);
	static double rms(const QVector<double> &data);
};

#endif // PECURVE_H
