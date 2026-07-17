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

#ifndef PECURVE_H
#define PECURVE_H

#include <QString>
#include <QStringList>
#include <QVector>

// Parameters controlling the RA periodic-error reconstruction.
struct PECurveOptions {
	double ratePxPerS = 0.0;   // RA guide rate (pixels per second of guide pulse)
	double decDeg = 0.0;       // target declination, for the cos(dec) pulse scale
	bool arcsec = true;        // output unit: arcsec (true) or pixels (false)
	bool excludeDither = true; // drop dithering rows and interpolate the gaps
	bool removeDrift = false;  // subtract the linear drift trend (isolate the PE)
	bool linearDetrend = false; // use a plain straight-line fit for the drift
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
// recovered by undoing the corrections. The INDIGO guider always drives the
// correction opposite to the drift (response = -gain*drift, see
// indigo_guider_reponse), so the cumulative corrections are subtracted:
//
//     PE(n) = residual(n) - sum_{k<=n} ( correction_seconds(k) * rate )
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

	// Removes the linear drift trend (and DC offset) from y over x. For a signal
	// dominated by the periodic error, a plain line fit is biased by the sinusoid
	// (it tilts a symmetric PE when the window is not a whole number of worm
	// periods), so this jointly fits a line plus one sinusoid at the best-fit
	// fundamental and subtracts only the line. Falls back to a plain line for
	// short series. Returns the input unchanged if it can't fit (fewer than two
	// points, mismatched sizes, or all x equal).
	static QVector<double> detrend(const QVector<double> &x, const QVector<double> &y);

	// Subtracts a plain least-squares straight-line fit of y over x (the classic
	// linear detrend). Unlike detrend() it does not model the periodic error, so
	// it can tilt a symmetric PE; kept as an explicit user-selectable option and
	// as a cross-check. Same "can't fit" fallbacks as detrend().
	static QVector<double> detrendLinear(const QVector<double> &x, const QVector<double> &y);

	// Peak-to-peak (max-min) and RMS of a series; 0 for empty input.
	static double peakToPeak(const QVector<double> &data);
	static double rms(const QVector<double> &data);
};

#endif // PECURVE_H
