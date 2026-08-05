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

	// Two option sets that compare equal produce the same reconstruction, which
	// is what lets PEAnalysis reuse a cached result across windows.
	bool operator==(const PECurveOptions &o) const {
		return ratePxPerS == o.ratePxPerS && decDeg == o.decDeg && arcsec == o.arcsec &&
		       excludeDither == o.excludeDither && removeDrift == o.removeDrift &&
		       linearDetrend == o.linearDetrend;
	}
	bool operator!=(const PECurveOptions &o) const { return !(*this == o); }
};

// Numeric view of one session's rows: exactly the columns the periodic-error
// reconstruction reads, decoded out of the log's text rows once.
//
// The windows re-run reconstruct() whenever a unit, calibration or detrend
// setting changes, and the main window re-pushes the rows on every pan/zoom of
// the graph. Parsing the strings on each of those passes dominated the cost of
// the whole tool (timestamps especially), so it happens once per row set here
// and the reconstruction then works on plain doubles.
struct PESamples {
	QString message;             // human-readable reason when !isValid()
	bool usedTime = false;       // x is elapsed seconds rather than a sample index
	bool hasArcsecScale = false; // the log paired an arcsec column with the pixel one
	double arcsecPerPx = 1.0;    // median arcsec/px over the paired columns
	QVector<double> x;           // elapsed seconds, or the sample index
	QVector<double> raPx;        // RA residual, px; NaN when the field did not parse
	QVector<double> raCorr;      // RA correction, seconds; NaN when it did not parse
	QVector<bool> dither;        // true on a dithering row (empty if the log has no such column)

	bool isValid() const { return !x.isEmpty(); }
	int count() const { return x.size(); }

	// Decodes rows against headers. On a log missing the RA residual or RA
	// correction column the result is empty and message says which.
	static PESamples fromRows(const QStringList &headers, const QVector<QStringList> &rows);
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

// Amplitude spectrum of a reconstructed PE curve. freq/amplitude run from DC
// (index 0) to the Nyquist frequency; amplitude is in the same unit as the
// series the FFT was computed from.
struct PEFFTData {
	bool valid = false;
	QString message; // human-readable reason when !valid
	QVector<double> freq;
	QVector<double> amplitude;
	double sampleRateHz = 0.0;
	double naturalResolutionHz = 0.0; // 1/duration, the un-padded bin width
};

// One detected spectral peak: the fundamental (harmonic == 1) or one of its
// integer multiples.
struct PEFFTPeak {
	int harmonic = 1;
	double frequencyHz = 0.0;
	double periodS = 0.0;
	double amplitude = 0.0;
	double relativeAmplitude = 0.0; // fraction of the fundamental's amplitude (0..1)
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
	// Reconstructs from already-decoded samples. Prefer this when the same rows
	// are reconstructed more than once (different units / detrend settings), so
	// the log's text is parsed only on the way into PESamples.
	static PECurveData reconstruct(const PESamples &samples, const PECurveOptions &options);

	// Convenience overload that decodes the rows first. Equivalent to
	// reconstruct(PESamples::fromRows(headers, rows), options).
	static PECurveData reconstruct(const QStringList &headers,
	                               const QVector<QStringList> &rows,
	                               const PECurveOptions &options);

	// Centered moving average; the window shrinks towards the ends so every
	// point stays defined. Returns the input unchanged for tiny sets / window<3.
	static QVector<double> smooth(const QVector<double> &data, int window);

	// An odd smoothing window (in samples) for smooth(), sized to strip the
	// per-frame seeing / centroid noise without eating into the periodic error
	// itself. A centred boxcar of duration D scales a sinusoid of period P by
	// sinc(D/P), which is exactly zero at D == P, so the window is capped at a
	// twentieth of fundamentalPeriod -- 99.6% of the fundamental survives, 94%
	// of its fourth harmonic. Pass 0 for fundamentalPeriod when none was
	// detected and the cap falls back to a fixed span of x instead. x is the
	// series' own axis (elapsed seconds, or the sample index on a log without
	// timestamps) and fundamentalPeriod is in those same units. Never returns
	// less than 3 or more than about a quarter of the series.
	static int autoSmoothWindow(const QVector<double> &x, double fundamentalPeriod);

	// The longest period worth searching for in a series spanning spanX: a
	// period needs several cycles before it can be told apart from drift, and no
	// worm period comes near the absolute cap anyway. Pass 0 or less for spanX
	// to get that cap. Feed the result to findHarmonics()' maxPeriodS.
	static double maxSearchPeriod(double spanX);

	// The fundamental's period, in the spectrum's own x units, from an already
	// computed spectrum of a series spanning spanX; 0 when no peak was found.
	// Convenience wrapper over findHarmonics() for callers that only want the
	// fundamental (cf. the smoothing window above).
	static double fundamentalPeriod(const PEFFTData &fft, double spanX);

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

	// Amplitude spectrum of an (approximately) uniformly time-sampled series (x
	// in elapsed seconds). Resamples onto a uniform grid at the median sample
	// interval, removes the DC mean, applies a Hamming window to reduce
	// spectral leakage, and zero-pads before an FFT so the peaks in
	// findHarmonics() can be located more precisely. Invalid (with a message)
	// for fewer than 8 samples or non-advancing timestamps.
	static PEFFTData computeFFT(const QVector<double> &x, const QVector<double> &y);

	// Finds the fundamental (the strongest local-maximum peak in the amplitude
	// spectrum, restricted to a period of at most maxPeriodS when maxPeriodS >
	// 0 so long-period drift leakage near DC can't hijack it) and every other
	// local-maximum peak at or above the fundamental's frequency whose
	// amplitude is at least minRelativeAmplitude of the fundamental's (default
	// 0.4 = 40%); non-peak bins (not a strict local maximum) are never
	// returned. Each peak's frequency/amplitude is refined to sub-bin
	// precision by a quadratic fit through its neighbouring bins. harmonic is
	// the peak's frequency rounded to the nearest multiple of the
	// fundamental's, for display only. Returns an empty list when fft is
	// invalid.
	static QVector<PEFFTPeak> findHarmonics(const PEFFTData &fft, double minRelativeAmplitude = 0.4,
	                                        double maxPeriodS = 0.0);

	// Writes x/residual/pe to fileName as CSV (one header row, then one row per
	// sample). xLabel and unitLabel are used for the header only. Returns true
	// on success; on failure returns false and, if errorMessage is non-null,
	// fills it with a human-readable reason.
	static bool saveCsv(const QString &fileName,
	                    const QVector<double> &x,
	                    const QVector<double> &residual,
	                    const QVector<double> &pe,
	                    bool usedTime,
	                    const QString &xLabel,
	                    const QString &unitLabel,
	                    QString *errorMessage = nullptr);

	// Writes an amplitude spectrum to fileName as CSV: one header row, then one
	// row per bin ordered by increasing period, matching the spectrum window's
	// axis. The DC bin is skipped (its period is infinite). The whole spectrum
	// is written, not just the region the window happens to be zoomed to.
	//
	// normalizeBy is the amplitude the relative-amplitude column is measured
	// against (the fundamental's); pass 0 or less to leave that column out.
	// usedTime picks the period/frequency units (seconds and Hz, or samples and
	// cycles per sample), and amplitudeUnit labels the amplitude column. Same
	// success/failure contract as saveCsv().
	static bool saveSpectrumCsv(const QString &fileName,
	                            const PEFFTData &fft,
	                            double normalizeBy,
	                            bool usedTime,
	                            const QString &amplitudeUnit,
	                            QString *errorMessage = nullptr);
};

#endif // PECURVE_H
