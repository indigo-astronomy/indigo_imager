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

#include "pecurve.h"

#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>

#include "guidelogstats.h"
#include "guidelogtime.h"

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// Longest period the fundamental search will ever consider, in seconds. No
// mount's worm period comes close; past this a "peak" is drift leaking towards
// DC rather than periodic error.
constexpr double kMaxSearchPeriodS = 1600.0;
// A period has to repeat a few times over the loaded window before it can be
// told apart from that drift.
constexpr double kMinCyclesToDetect = 3.0;
// The smoothing window is held to this fraction of the fundamental's period.
constexpr double kSmoothPeriodDivisor = 20.0;
// Smoothing window when no fundamental was detected: seconds of data, which is
// already far longer than seeing and centroid noise stay correlated for. On a
// log without timestamps x counts samples, so it reads as a sample count.
constexpr double kSmoothFallbackSpan = 10.0;

// Median of a copy of the samples (returns 0 when empty). Uses nth_element
// rather than a full sort: only the middle order statistic is wanted.
double median(QVector<double> samples) {
	if (samples.isEmpty()) {
		return 0.0;
	}
	const int mid = samples.size() / 2;
	std::nth_element(samples.begin(), samples.begin() + mid, samples.end());
	const double hi = samples.at(mid);
	if (samples.size() % 2 != 0) {
		return hi;
	}
	// Even count: the other middle element is the largest of the lower half,
	// which nth_element has already partitioned to the left.
	const double lo = *std::max_element(samples.begin(), samples.begin() + mid);
	return 0.5 * (lo + hi);
}

// Refines a spectral peak at bin k using quadratic (parabolic) interpolation
// of the amplitude in the surrounding bins, since the true peak generally
// falls between two FFT bins. Returns the fractional bin offset (-0.5..0.5)
// and the interpolated peak amplitude; falls back to no offset at the array
// edges or a degenerate (flat) neighbourhood.
struct ParabolicPeak {
	double binOffset = 0.0;
	double amplitude = 0.0;
};

ParabolicPeak parabolicInterpolate(const QVector<double> &amplitude, int k) {
	ParabolicPeak result;
	if (k <= 0 || k >= amplitude.size() - 1) {
		result.amplitude = amplitude.at(k);
		return result;
	}
	const double alpha = amplitude.at(k - 1);
	const double beta = amplitude.at(k);
	const double gamma = amplitude.at(k + 1);
	const double denom = alpha - 2.0 * beta + gamma;
	if (std::fabs(denom) < 1e-15) {
		result.amplitude = beta;
		return result;
	}
	const double p = std::max(-1.0, std::min(1.0, 0.5 * (alpha - gamma) / denom));
	result.binOffset = p;
	result.amplitude = beta - 0.25 * (alpha - gamma) * p;
	return result;
}

// Solves the 4x4 system a*z = b in place (Gaussian elimination with partial
// pivoting). On success b holds the solution; returns false if singular. Fixed
// at 4x4 and taken by plain arrays so the detrend search below can keep it on
// the stack — it is called a few hundred times per detrend.
bool solve4(double a[4][4], double b[4]) {
	for (int col = 0; col < 4; col++) {
		int pivot = col;
		double best = std::fabs(a[col][col]);
		for (int r = col + 1; r < 4; r++) {
			if (std::fabs(a[r][col]) > best) {
				best = std::fabs(a[r][col]);
				pivot = r;
			}
		}
		if (best < 1e-12) {
			return false;
		}
		if (pivot != col) {
			for (int c = 0; c < 4; c++) {
				std::swap(a[pivot][c], a[col][c]);
			}
			std::swap(b[pivot], b[col]);
		}
		for (int r = col + 1; r < 4; r++) {
			const double f = a[r][col] / a[col][col];
			for (int c = col; c < 4; c++) {
				a[r][c] -= f * a[col][c];
			}
			b[r] -= f * b[col];
		}
	}
	for (int row = 3; row >= 0; row--) {
		double s = b[row];
		for (int c = row + 1; c < 4; c++) {
			s -= a[row][c] * b[c];
		}
		b[row] = s / a[row][row];
	}
	return true;
}

// Smallest power of two >= n (n >= 1).
int nextPow2(int n) {
	int p = 1;
	while (p < n) {
		p <<= 1;
	}
	return p;
}

// In-place iterative radix-2 Cooley-Tukey FFT (decimation in time). a.size()
// must be a power of two.
void fftRadix2(QVector<std::complex<double>> &a) {
	const int n = a.size();
	if (n <= 1) {
		return;
	}
	for (int i = 1, j = 0; i < n; i++) {
		int bit = n >> 1;
		for (; j & bit; bit >>= 1) {
			j ^= bit;
		}
		j ^= bit;
		if (i < j) {
			std::swap(a[i], a[j]);
		}
	}
	for (int len = 2; len <= n; len <<= 1) {
		const double ang = -kTwoPi / len;
		const std::complex<double> wlen(std::cos(ang), std::sin(ang));
		for (int i = 0; i < n; i += len) {
			std::complex<double> w(1.0, 0.0);
			for (int k = 0; k < len / 2; k++) {
				const std::complex<double> u = a[i + k];
				const std::complex<double> v = a[i + k + len / 2] * w;
				a[i + k] = u + v;
				a[i + k + len / 2] = u - v;
				w *= wlen;
			}
		}
	}
}

} // namespace

PESamples PESamples::fromRows(const QStringList &headers, const QVector<QStringList> &rows) {
	PESamples out;

	const GuideColumns columns(headers);
	if (rows.isEmpty() || columns.raPixel < 0) {
		out.message = "No RA residual column ('RA Dif') in this log.";
		return out;
	}
	if (columns.raCorr < 0) {
		out.message = "No RA correction column ('RA Corr') in this log.";
		return out;
	}

	const int n = rows.size();

	// X is elapsed seconds when the log's first row carries a usable timestamp,
	// otherwise the plain sample index.
	qint64 firstMs = 0;
	const bool useTime = columns.timestamp >= 0 &&
	                     GuideLogTime::parseMSecs(rows.first().at(columns.timestamp), &firstMs);
	out.usedTime = useTime;

	out.x.resize(n);
	out.raPx.resize(n);
	out.raCorr.resize(n);
	if (columns.dither >= 0) {
		out.dither.resize(n);
	}

	QVector<double> ratios;
	const bool wantArcsec = columns.raArc >= 0;
	if (wantArcsec) {
		ratios.reserve(n);
	}

	for (int i = 0; i < n; ++i) {
		const QStringList &row = rows.at(i);

		double x = static_cast<double>(i);
		if (useTime) {
			qint64 ms = 0;
			if (GuideLogTime::parseMSecs(row.at(columns.timestamp), &ms)) {
				x = (ms - firstMs) / 1000.0;
			}
		}
		out.x[i] = x;

		bool pxOk = false;
		const double px = row.at(columns.raPixel).toDouble(&pxOk);
		out.raPx[i] = pxOk ? px : kNaN;

		bool corrOk = false;
		const double corr = row.at(columns.raCorr).toDouble(&corrOk);
		out.raCorr[i] = corrOk ? corr : kNaN;

		if (columns.dither >= 0) {
			bool dOk = false;
			const double dv = row.at(columns.dither).toDouble(&dOk);
			out.dither[i] = (dOk && dv != 0.0);
		}

		if (wantArcsec && pxOk && std::abs(px) > 0.05) {
			bool arcOk = false;
			const double arc = row.at(columns.raArc).toDouble(&arcOk);
			if (arcOk) {
				ratios.append(arc / px);
			}
		}
	}

	if (wantArcsec) {
		const double m = median(ratios);
		if (m > 0.0) {
			out.arcsecPerPx = m;
			out.hasArcsecScale = true;
		}
	}
	return out;
}

PECurveData PECurve::reconstruct(const QStringList &headers,
                                 const QVector<QStringList> &rows,
                                 const PECurveOptions &options) {
	return reconstruct(PESamples::fromRows(headers, rows), options);
}

PECurveData PECurve::reconstruct(const PESamples &samples, const PECurveOptions &options) {
	PECurveData out;

	if (!samples.isValid()) {
		out.message = samples.message.isEmpty()
		                  ? QStringLiteral("No usable RA samples in this session.")
		                  : samples.message;
		return out;
	}

	const double rate = options.ratePxPerS; // px/s
	out.hasRate = (rate > 0.0);
	// The driver applies pulses of correction/(SPEED_RA*cos_dec), so the star
	// actually moves correction*SPEED_RA*cos_dec. Undo that same cos(dec) scale.
	const double cosDec = std::cos(options.decDeg * 0.017453292519943295); // deg->rad
	const double unitScale = options.arcsec ? samples.arcsecPerPx : 1.0;
	const bool useTime = samples.usedTime;

	// First pass: compute the reconstruction per row. Dithering rows (and any
	// unparseable rows) are marked as gaps: they neither advance the cumulative
	// correction nor emit a value, so the deliberate dither moves stay out of
	// the periodic error. X is elapsed seconds when timestamps are available.
	const int n = samples.count();
	const bool haveDither = options.excludeDither && samples.dither.size() == n;
	const QVector<double> &rowX = samples.x;
	QVector<double> rowRes(n, 0.0);
	QVector<double> rowPe(n, 0.0);
	QVector<bool> kept(n, false);

	double cumCorrPx = 0.0;
	for (int i = 0; i < n; ++i) {
		const double resPx = samples.raPx.at(i);
		if ((haveDither && samples.dither.at(i)) || std::isnan(resPx)) {
			continue; // gap: do not advance the cumulative correction
		}

		const double corrSec = samples.raCorr.at(i);
		if (!std::isnan(corrSec)) {
			// Corrections oppose the drift, so undo them by subtracting.
			cumCorrPx -= corrSec * rate * cosDec;
		}
		rowRes[i] = resPx * unitScale;
		rowPe[i] = (resPx + cumCorrPx) * unitScale;
		kept[i] = true;
	}

	int firstKept = -1;
	int lastKept = -1;
	for (int i = 0; i < n; ++i) {
		if (kept[i]) {
			if (firstKept < 0) {
				firstKept = i;
			}
			lastKept = i;
		}
	}
	if (firstKept < 0) {
		out.message = "No usable RA samples in this session.";
		return out;
	}

	// Second pass: emit a continuous series. Kept rows pass through; interior
	// gaps are linearly interpolated (in time) from their nearest kept
	// neighbours. Leading / trailing gaps are dropped (no extrapolation).
	out.x.reserve(n);
	out.residual.reserve(n);
	out.pe.reserve(n);

	int prevKept = -1;
	for (int i = firstKept; i <= lastKept; ++i) {
		if (kept[i]) {
			out.x.append(rowX[i]);
			out.residual.append(rowRes[i]);
			out.pe.append(rowPe[i]);
			prevKept = i;
			continue;
		}
		int nextKept = -1;
		for (int j = i + 1; j <= lastKept; ++j) {
			if (kept[j]) {
				nextKept = j;
				break;
			}
		}
		const double x0 = rowX[prevKept];
		const double x1 = rowX[nextKept];
		const double t = (x1 > x0) ? (rowX[i] - x0) / (x1 - x0) : 0.0;
		out.x.append(rowX[i]);
		out.residual.append(rowRes[prevKept] + (rowRes[nextKept] - rowRes[prevKept]) * t);
		out.pe.append(rowPe[prevKept] + (rowPe[nextKept] - rowPe[prevKept]) * t);
	}

	// Optionally remove the linear drift (e.g. from polar-alignment error) so the
	// periodic error is not swamped by a slope. Only the reconstructed PE carries
	// that drift (it accumulates in the corrections); the residual is left as
	// measured so its RMS still matches the guided error shown elsewhere.
	if (options.removeDrift) {
		out.pe = options.linearDetrend ? detrendLinear(out.x, out.pe)
		                               : detrend(out.x, out.pe);
	}

	out.valid = true;
	out.usedTime = useTime;
	return out;
}

QVector<double> PECurve::detrend(const QVector<double> &x, const QVector<double> &y) {
	const int n = y.size();
	if (n < 2 || x.size() != n) {
		return y;
	}

	const double span = x.last() - x.first();
	if (std::fabs(span) < 1e-12) {
		return y; // all x equal — no line to fit
	}

	// Fit in normalised time u = (x - mean) / span, so u is centred on 0 and
	// spans about [-0.5, 0.5]. This keeps the normal equations well conditioned
	// and, because mean(u) = 0, simplifies the plain-line fit.
	double xMean = 0.0;
	for (int i = 0; i < n; ++i) {
		xMean += x[i];
	}
	xMean /= n;
	QVector<double> u(n);
	for (int i = 0; i < n; ++i) {
		u[i] = (x[i] - xMean) / span;
	}

	// Baseline: an ordinary least-squares straight line c0 + c1*u.
	double suy = 0.0, suu = 0.0, sy = 0.0, syy = 0.0;
	for (int i = 0; i < n; ++i) {
		sy += y[i];
		suy += u[i] * y[i];
		suu += u[i] * u[i];
		syy += y[i] * y[i];
	}
	double bestC0 = sy / n;
	double bestC1 = (suu > 1e-12) ? (suy / suu) : 0.0;

	// A straight-line fit to a signal dominated by the periodic error is biased
	// by the sinusoid itself unless the window spans a whole number of worm
	// periods — which tilts a symmetric PE. To avoid that, jointly fit a line
	// plus one sinusoid at the best-fit fundamental, then subtract only the
	// line. The fundamental is found by scanning candidate periods (expressed as
	// k = cycles across the window) and keeping the one with the smallest fit
	// residual.
	//
	// The scan is the most expensive thing the PE tools do, so three things keep
	// it cheap without changing what it computes:
	//
	//  - sin/cos are not re-evaluated per candidate. Stepping k by kStep rotates
	//    the phasor (sin(w*u), cos(w*u)) by a fixed per-sample angle, so each
	//    step is one complex multiply per sample. The phasor is re-seeded from
	//    sin/cos every kReseed steps to keep rounding from accumulating.
	//  - Only the sin/cos rows of the normal equations vary with k; the {1, u}
	//    block and the right-hand side entries above it are loop invariants.
	//  - The residual sum of squares comes from the normal equations themselves
	//    (RSS = y'y - c'X'y holds at the least-squares solution), so there is no
	//    second pass over the samples to evaluate the model.
	if (n >= 8) {
		constexpr double kStep = 0.1;
		constexpr int kReseed = 32;
		// Highest frequency worth trying: keep at least ~5 samples per cycle, and
		// cap at 40 cycles so a slow worm's drift search stays cheap.
		const double kMax = std::min(40.0, static_cast<double>(n - 1) / 5.0);
		const int steps = static_cast<int>((kMax - 1.0) / kStep + 1e-9) + 1;

		QVector<double> sinCur(n), cosCur(n), sinStep(n), cosStep(n);
		for (int i = 0; i < n; ++i) {
			const double delta = kTwoPi * kStep * u[i];
			sinStep[i] = std::sin(delta);
			cosStep[i] = std::cos(delta);
		}

		double bestRss = -1.0;
		for (int step = 0; step < steps; ++step) {
			if (step % kReseed == 0) {
				const double w = kTwoPi * (1.0 + step * kStep);
				for (int i = 0; i < n; ++i) {
					sinCur[i] = std::sin(w * u[i]);
					cosCur[i] = std::cos(w * u[i]);
				}
			}

			// Normal equations for the basis {1, u, sin(w*u), cos(w*u)}. Note
			// mean(u) == 0 by construction, so the 1-vs-u entries vanish.
			double a02 = 0.0, a03 = 0.0, a12 = 0.0, a13 = 0.0;
			double a22 = 0.0, a23 = 0.0, a33 = 0.0, b2 = 0.0, b3 = 0.0;
			for (int i = 0; i < n; ++i) {
				const double s = sinCur[i];
				const double c = cosCur[i];
				const double ui = u[i];
				const double yi = y[i];
				a02 += s;
				a03 += c;
				a12 += ui * s;
				a13 += ui * c;
				a22 += s * s;
				a23 += s * c;
				a33 += c * c;
				b2 += s * yi;
				b3 += c * yi;
			}

			double a[4][4] = {
				{static_cast<double>(n), 0.0, a02, a03},
				{0.0, suu, a12, a13},
				{a02, a12, a22, a23},
				{a03, a13, a23, a33},
			};
			const double rhs[4] = {sy, suy, b2, b3};
			double coeff[4] = {rhs[0], rhs[1], rhs[2], rhs[3]};
			if (solve4(a, coeff)) {
				const double rss = syy - (coeff[0] * rhs[0] + coeff[1] * rhs[1] +
				                          coeff[2] * rhs[2] + coeff[3] * rhs[3]);
				if (bestRss < 0.0 || rss < bestRss) {
					bestRss = rss;
					bestC0 = coeff[0];
					bestC1 = coeff[1];
				}
			}

			// Advance the phasor to the next candidate, unless the next step
			// re-seeds it from scratch anyway.
			if (step + 1 < steps && (step + 1) % kReseed != 0) {
				for (int i = 0; i < n; ++i) {
					const double s = sinCur[i];
					const double c = cosCur[i];
					sinCur[i] = s * cosStep[i] + c * sinStep[i];
					cosCur[i] = c * cosStep[i] - s * sinStep[i];
				}
			}
		}
	}

	QVector<double> out(n);
	for (int i = 0; i < n; ++i) {
		out[i] = y[i] - (bestC0 + bestC1 * u[i]);
	}
	return out;
}

QVector<double> PECurve::detrendLinear(const QVector<double> &x, const QVector<double> &y) {
	const int n = y.size();
	if (n < 2 || x.size() != n) {
		return y;
	}
	double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
	for (int i = 0; i < n; ++i) {
		sx += x[i];
		sy += y[i];
		sxx += x[i] * x[i];
		sxy += x[i] * y[i];
	}
	const double denom = n * sxx - sx * sx;
	if (std::fabs(denom) < 1e-12) {
		return y; // all x equal — no line to fit
	}
	const double slope = (n * sxy - sx * sy) / denom;
	const double intercept = (sy - slope * sx) / n;
	QVector<double> out(n);
	for (int i = 0; i < n; ++i) {
		out[i] = y[i] - (intercept + slope * x[i]);
	}
	return out;
}

QVector<double> PECurve::smooth(const QVector<double> &data, int window) {
	const int n = data.size();
	if (n < 3 || window < 3) {
		return data;
	}
	const int half = window / 2;
	QVector<double> out(n);
	for (int i = 0; i < n; ++i) {
		const int lo = std::max(0, i - half);
		const int hi = std::min(n - 1, i + half);
		double sum = 0.0;
		for (int k = lo; k <= hi; ++k) {
			sum += data.at(k);
		}
		out[i] = sum / (hi - lo + 1);
	}
	return out;
}

double PECurve::maxSearchPeriod(double spanX) {
	if (spanX <= 0.0) {
		return kMaxSearchPeriodS;
	}
	return std::min(kMaxSearchPeriodS, spanX / kMinCyclesToDetect);
}

double PECurve::fundamentalPeriod(const PEFFTData &fft, double spanX) {
	const QVector<PEFFTPeak> peaks = findHarmonics(fft, 0.4, maxSearchPeriod(spanX));
	return peaks.isEmpty() ? 0.0 : peaks.first().periodS;
}

int PECurve::autoSmoothWindow(const QVector<double> &x, double fundamentalPeriod) {
	const int n = x.size();
	if (n < 5) {
		return 3;
	}

	// Median step rather than the average, so a dither gap (or a stretch of
	// dropped frames) does not inflate it and shrink the window.
	QVector<double> steps;
	steps.reserve(n - 1);
	for (int i = 1; i < n; ++i) {
		const double dx = x.at(i) - x.at(i - 1);
		if (dx > 0.0) {
			steps.append(dx);
		}
	}
	const double step = steps.isEmpty() ? 1.0 : median(steps);
	if (!(step > 0.0)) {
		return 3;
	}

	// The span to average over is set by the periodic error itself, never by how
	// much of the log happens to be loaded: a window that grew with the sample
	// count would attenuate -- and past D == P outright invert -- the very
	// periodic error the curve is meant to show.
	const double span = (fundamentalPeriod > 0.0) ? fundamentalPeriod / kSmoothPeriodDivisor
	                                              : kSmoothFallbackSpan;

	int window = static_cast<int>(std::lround(span / step));
	if (window % 2 == 0) {
		window++; // odd, so the moving average stays centred on its sample
	}
	window = std::min(window, (n / 4) | 1); // never average away a quarter of the series
	return std::max(3, window);
}

double PECurve::peakToPeak(const QVector<double> &data) {
	if (data.isEmpty()) {
		return 0.0;
	}
	double lo = data.first();
	double hi = data.first();
	for (double v : data) {
		lo = std::min(lo, v);
		hi = std::max(hi, v);
	}
	return hi - lo;
}

double PECurve::rms(const QVector<double> &data) {
	if (data.isEmpty()) {
		return 0.0;
	}
	double sumSq = 0.0;
	for (double v : data) {
		sumSq += v * v;
	}
	return std::sqrt(sumSq / data.size());
}

PEFFTData PECurve::computeFFT(const QVector<double> &x, const QVector<double> &y) {
	PEFFTData out;
	const int n = y.size();
	if (n < 8 || x.size() != n) {
		out.message = "Not enough samples for an FFT.";
		return out;
	}

	// The FFT assumes evenly spaced samples; guiding logs have small timing
	// jitter, so resample onto a uniform grid at the median sample interval.
	QVector<double> dts;
	dts.reserve(n - 1);
	for (int i = 1; i < n; i++) {
		const double d = x.at(i) - x.at(i - 1);
		if (d > 0.0) {
			dts.append(d);
		}
	}
	if (dts.isEmpty()) {
		out.message = "Samples do not advance in time.";
		return out;
	}
	const double dt = median(dts);
	const double duration = x.last() - x.first();
	const int n0 = static_cast<int>(duration / dt) + 1;
	if (n0 < 8) {
		out.message = "Session is too short for an FFT.";
		return out;
	}

	QVector<double> resampled(n0);
	int srcIdx = 0;
	for (int k = 0; k < n0; k++) {
		const double t = x.first() + k * dt;
		while (srcIdx + 1 < n - 1 && x.at(srcIdx + 1) < t) {
			srcIdx++;
		}
		const int i0 = srcIdx;
		const int i1 = std::min(i0 + 1, n - 1);
		const double x0 = x.at(i0);
		const double x1 = x.at(i1);
		const double frac = (x1 > x0) ? (t - x0) / (x1 - x0) : 0.0;
		resampled[k] = y.at(i0) + (y.at(i1) - y.at(i0)) * frac;
	}

	// Remove the DC mean, then apply a Hamming window (reduces spectral leakage
	// from the curve's non-periodic edges at the cost of some frequency
	// resolution, an acceptable trade-off for picking out the worm harmonics).
	double mean = 0.0;
	for (double v : resampled) {
		mean += v;
	}
	mean /= n0;

	QVector<double> windowed(n0);
	double sumWindow = 0.0;
	for (int k = 0; k < n0; k++) {
		const double w = (n0 > 1) ? (0.54 - 0.46 * std::cos(kTwoPi * k / (n0 - 1))) : 1.0;
		windowed[k] = (resampled.at(k) - mean) * w;
		sumWindow += w;
	}
	if (sumWindow <= 0.0) {
		out.message = "Session is too short for an FFT.";
		return out;
	}

	// Zero-pad well beyond n0 (capped) so the peaks below can be located more
	// precisely than the curve's natural (un-padded) frequency resolution.
	// Padding only interpolates the spectrum, it adds no real resolution — see
	// PEFFTData::naturalResolutionHz for what the session can actually resolve.
	constexpr int kPadFactor = 4;
	constexpr int kMaxFFTSize = 1 << 17;
	int nfft = nextPow2(n0);
	while (nfft < n0 * kPadFactor && nfft < kMaxFFTSize) {
		nfft <<= 1;
	}

	QVector<std::complex<double>> spectrum(nfft, std::complex<double>(0.0, 0.0));
	for (int k = 0; k < n0; k++) {
		spectrum[k] = std::complex<double>(windowed.at(k), 0.0);
	}
	fftRadix2(spectrum);

	const double sampleRate = 1.0 / dt;
	const int bins = nfft / 2 + 1;
	out.freq.resize(bins);
	out.amplitude.resize(bins);
	for (int k = 0; k < bins; k++) {
		const double mag = std::abs(spectrum.at(k));
		out.amplitude[k] = (k == 0) ? (mag / sumWindow) : (2.0 * mag / sumWindow);
		out.freq[k] = k * sampleRate / nfft;
	}
	out.sampleRateHz = sampleRate;
	out.naturalResolutionHz = 1.0 / duration;
	out.valid = true;
	return out;
}

QVector<PEFFTPeak> PECurve::findHarmonics(const PEFFTData &fft, double minRelativeAmplitude, double maxPeriodS) {
	QVector<PEFFTPeak> peaks;
	if (!fft.valid || fft.amplitude.size() < 3) {
		return peaks;
	}
	const double df = fft.freq.at(1) - fft.freq.at(0);
	if (df <= 0.0) {
		return peaks;
	}
	const double minFrequencyHz = (maxPeriodS > 0.0) ? (1.0 / maxPeriodS) : 0.0;

	// Only genuine local maxima count as peaks (excludes DC and the Nyquist
	// bin, which have no lower/upper neighbour to compare against), each
	// refined to sub-bin precision with a quadratic fit.
	struct RawPeak {
		double frequencyHz;
		double amplitude;
	};
	QVector<RawPeak> rawPeaks;
	for (int k = 1; k < fft.amplitude.size() - 1; k++) {
		const double v = fft.amplitude.at(k);
		if (v > fft.amplitude.at(k - 1) && v > fft.amplitude.at(k + 1)) {
			const ParabolicPeak refined = parabolicInterpolate(fft.amplitude, k);
			rawPeaks.append({(k + refined.binOffset) * df, refined.amplitude});
		}
	}
	if (rawPeaks.isEmpty()) {
		return peaks;
	}

	// Find the most pronounced (strongest) peak at or above minFrequencyHz,
	// so long-period drift leakage near DC can't influence the amplitude threshold.
	int mostPronounedIdx = -1;
	for (int i = 0; i < rawPeaks.size(); i++) {
		if (rawPeaks.at(i).frequencyHz < minFrequencyHz) {
			continue;
		}
		if (mostPronounedIdx < 0 || rawPeaks.at(i).amplitude > rawPeaks.at(mostPronounedIdx).amplitude) {
			mostPronounedIdx = i;
		}
	}
	if (mostPronounedIdx < 0) {
		return peaks;
	}
	const double mostPronouncedAmplitude = rawPeaks.at(mostPronounedIdx).amplitude;

	// The fundamental is the lowest frequency peak at or above minFrequencyHz
	// that is at least minRelativeAmplitude of the most pronounced peak.
	int fundIdx = -1;
	double minFoundFrequency = std::numeric_limits<double>::max();
	for (int i = 0; i < rawPeaks.size(); i++) {
		if (rawPeaks.at(i).frequencyHz < minFrequencyHz) {
			continue;
		}
		if (rawPeaks.at(i).amplitude >= minRelativeAmplitude * mostPronouncedAmplitude) {
			if (rawPeaks.at(i).frequencyHz < minFoundFrequency) {
				fundIdx = i;
				minFoundFrequency = rawPeaks.at(i).frequencyHz;
			}
		}
	}
	if (fundIdx < 0) {
		return peaks;
	}
	const RawPeak fund = rawPeaks.at(fundIdx);
	if (fund.amplitude <= 0.0 || fund.frequencyHz <= 0.0) {
		return peaks;
	}

	PEFFTPeak fundamental;
	fundamental.harmonic = 1;
	fundamental.frequencyHz = fund.frequencyHz;
	fundamental.periodS = 1.0 / fund.frequencyHz;
	fundamental.amplitude = fund.amplitude;
	fundamental.relativeAmplitude = fund.amplitude / mostPronouncedAmplitude;
	peaks.append(fundamental);

	std::sort(rawPeaks.begin(), rawPeaks.end(), [](const RawPeak &a, const RawPeak &b) {
		return a.frequencyHz < b.frequencyHz;
	});
	for (const RawPeak &rp : rawPeaks) {
		if (rp.frequencyHz < fund.frequencyHz - df * 0.5) {
			continue; // lower frequency than F0: not a harmonic, not labelled
		}
		if (std::fabs(rp.frequencyHz - fund.frequencyHz) < df * 0.5) {
			continue; // the fundamental itself
		}
		if (rp.amplitude < minRelativeAmplitude * mostPronouncedAmplitude) {
			continue; // below the 40% of strongest peak threshold
		}
		PEFFTPeak p;
		p.harmonic = std::max(2, static_cast<int>(std::lround(rp.frequencyHz / fund.frequencyHz)));
		p.frequencyHz = rp.frequencyHz;
		p.periodS = 1.0 / rp.frequencyHz;
		p.amplitude = rp.amplitude;
		p.relativeAmplitude = rp.amplitude / mostPronouncedAmplitude;
		peaks.append(p);
	}
	return peaks;
}

bool PECurve::saveCsv(const QString &fileName,
                      const QVector<double> &x,
                      const QVector<double> &residual,
                      const QVector<double> &pe,
                      bool usedTime,
                      const QString &xLabel,
                      const QString &unitLabel,
                      QString *errorMessage) {
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
		if (errorMessage) {
			*errorMessage = QObject::tr("Can not open file '%1' for writing.").arg(fileName);
		}
		return false;
	}

	QTextStream ts(&file);
	ts << xLabel << "," << "Residual (" << unitLabel << ")," << "PE (" << unitLabel << ")\n";
	for (int i = 0; i < x.size(); ++i) {
		const double res = (i < residual.size()) ? residual.at(i) : 0.0;
		const double val = (i < pe.size()) ? pe.at(i) : 0.0;
		ts << QString::number(x.at(i), 'f', usedTime ? 3 : 0)
		   << "," << QString::number(res, 'f', 6)
		   << "," << QString::number(val, 'f', 6) << "\n";
	}
	file.close();
	return true;
}

bool PECurve::saveSpectrumCsv(const QString &fileName,
                              const PEFFTData &fft,
                              double normalizeBy,
                              bool usedTime,
                              const QString &amplitudeUnit,
                              QString *errorMessage) {
	if (!fft.valid || fft.freq.size() < 2) {
		if (errorMessage) {
			*errorMessage = QObject::tr("There is no spectrum to export yet.");
		}
		return false;
	}

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
		if (errorMessage) {
			*errorMessage = QObject::tr("Can not open file '%1' for writing.").arg(fileName);
		}
		return false;
	}

	const QString periodUnit = usedTime ? QStringLiteral("s") : QStringLiteral("samples");
	const QString freqUnit = usedTime ? QStringLiteral("Hz") : QStringLiteral("cycles/sample");
	const bool relative = normalizeBy > 0.0;

	QTextStream ts(&file);
	ts << "Period (" << periodUnit << ")," << "Frequency (" << freqUnit << "),"
	   << "Amplitude (" << amplitudeUnit << ")";
	if (relative) {
		ts << ",Relative amplitude";
	}
	ts << "\n";

	// Walked from the Nyquist end down so period increases, as on the plot. Bin
	// 0 is DC and has no finite period, so it is left out.
	for (int k = fft.freq.size() - 1; k >= 1; --k) {
		const double f = fft.freq.at(k);
		if (f <= 0.0) {
			continue;
		}
		const double amplitude = fft.amplitude.at(k);
		ts << QString::number(1.0 / f, 'f', 6)
		   << "," << QString::number(f, 'g', 10)
		   << "," << QString::number(amplitude, 'f', 6);
		if (relative) {
			ts << "," << QString::number(amplitude / normalizeBy, 'f', 6);
		}
		ts << "\n";
	}
	file.close();
	return true;
}
