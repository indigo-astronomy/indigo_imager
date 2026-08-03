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

#include <QDateTime>
#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <complex>

#include "guidelogstats.h"

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;

// Parses "yyyy-MM-dd HH:mm:ss.zzz" (optionally quoted/space-padded). Returns an
// invalid QDateTime on failure.
QDateTime parseTimestamp(QString value) {
	value = value.trimmed();
	if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2) {
		value = value.mid(1, value.size() - 2);
	}
	QDateTime dt = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss.zzz");
	if (!dt.isValid()) {
		dt = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");
	}
	return dt;
}

// Median of a copy of the samples (returns 0 when empty).
double median(QVector<double> samples) {
	if (samples.isEmpty()) {
		return 0.0;
	}
	std::sort(samples.begin(), samples.end());
	const int mid = samples.size() / 2;
	if (samples.size() % 2 == 0) {
		return 0.5 * (samples.at(mid - 1) + samples.at(mid));
	}
	return samples.at(mid);
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

// Solves the m x m linear system a*z = b in place (Gaussian elimination with
// partial pivoting). On success b holds the solution; returns false if singular.
bool solveLinearSystem(QVector<QVector<double>> &a, QVector<double> &b) {
	const int m = b.size();
	for (int col = 0; col < m; col++) {
		int pivot = col;
		double best = std::fabs(a[col][col]);
		for (int r = col + 1; r < m; r++) {
			if (std::fabs(a[r][col]) > best) {
				best = std::fabs(a[r][col]);
				pivot = r;
			}
		}
		if (best < 1e-12) {
			return false;
		}
		if (pivot != col) {
			a[pivot].swap(a[col]);
			std::swap(b[pivot], b[col]);
		}
		for (int r = col + 1; r < m; r++) {
			const double f = a[r][col] / a[col][col];
			for (int c = col; c < m; c++) {
				a[r][c] -= f * a[col][c];
			}
			b[r] -= f * b[col];
		}
	}
	for (int row = m - 1; row >= 0; row--) {
		double s = b[row];
		for (int c = row + 1; c < m; c++) {
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

PECurveData PECurve::reconstruct(const QStringList &headers,
                                 const QVector<QStringList> &rows,
                                 const PECurveOptions &options) {
	PECurveData out;

	const GuideColumns columns(headers);
	if (rows.isEmpty() || columns.raPixel < 0) {
		out.message = "No RA residual column ('RA Dif') in this log.";
		return out;
	}
	if (columns.raCorr < 0) {
		out.message = "No RA correction column ('RA Corr') in this log.";
		return out;
	}

	const double rate = options.ratePxPerS; // px/s
	out.hasRate = (rate > 0.0);
	// The driver applies pulses of correction/(SPEED_RA*cos_dec), so the star
	// actually moves correction*SPEED_RA*cos_dec. Undo that same cos(dec) scale.
	const double cosDec = std::cos(options.decDeg * 0.017453292519943295); // deg->rad

	// arcsec-per-pixel scale, derived from the log's paired px / arcsec columns.
	double arcsecPerPx = 1.0;
	if (options.arcsec && columns.raArc >= 0) {
		QVector<double> ratios;
		ratios.reserve(rows.size());
		for (const QStringList &row : rows) {
			bool pxOk = false;
			bool arcOk = false;
			const double px = row.at(columns.raPixel).toDouble(&pxOk);
			const double arc = row.at(columns.raArc).toDouble(&arcOk);
			if (pxOk && arcOk && std::abs(px) > 0.05) {
				ratios.append(arc / px);
			}
		}
		const double m = median(ratios);
		if (m > 0.0) {
			arcsecPerPx = m;
		}
	}
	const double unitScale = options.arcsec ? arcsecPerPx : 1.0;

	QDateTime firstTs;
	if (columns.timestamp >= 0) {
		firstTs = parseTimestamp(rows.first().at(columns.timestamp));
	}
	const bool useTime = firstTs.isValid();

	// First pass: compute the reconstruction per row. Dithering rows (and any
	// unparseable rows) are marked as gaps: they neither advance the cumulative
	// correction nor emit a value, so the deliberate dither moves stay out of
	// the periodic error. X is elapsed seconds when timestamps are available.
	const int n = rows.size();
	QVector<double> rowX(n, 0.0);
	QVector<double> rowRes(n, 0.0);
	QVector<double> rowPe(n, 0.0);
	QVector<bool> kept(n, false);

	double cumCorrPx = 0.0;
	for (int i = 0; i < n; ++i) {
		const QStringList &row = rows.at(i);

		double x = static_cast<double>(i);
		if (useTime) {
			const QDateTime ts = parseTimestamp(row.at(columns.timestamp));
			if (ts.isValid()) {
				x = firstTs.msecsTo(ts) / 1000.0;
			}
		}
		rowX[i] = x;

		bool isDither = false;
		if (options.excludeDither && columns.dither >= 0) {
			bool dOk = false;
			const double dv = row.at(columns.dither).toDouble(&dOk);
			isDither = (dOk && dv != 0.0);
		}
		bool resOk = false;
		const double resPx = row.at(columns.raPixel).toDouble(&resOk);
		if (isDither || !resOk) {
			continue; // gap: do not advance the cumulative correction
		}

		bool corrOk = false;
		const double corrSec = row.at(columns.raCorr).toDouble(&corrOk);
		if (corrOk) {
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
	double suy = 0.0, suu = 0.0, sy = 0.0;
	for (int i = 0; i < n; ++i) {
		sy += y[i];
		suy += u[i] * y[i];
		suu += u[i] * u[i];
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
	if (n >= 8) {
		const double twoPi = 6.283185307179586;
		// Highest frequency worth trying: keep at least ~5 samples per cycle, and
		// cap at 40 cycles so a slow worm's drift search stays cheap.
		const double kMax = std::min(40.0, static_cast<double>(n - 1) / 5.0);
		double bestRss = -1.0;
		for (double k = 1.0; k <= kMax + 1e-9; k += 0.1) {
			const double w = twoPi * k;
			// Normal equations for the basis {1, u, sin(w*u), cos(w*u)}.
			QVector<QVector<double>> a(4, QVector<double>(4, 0.0));
			QVector<double> b(4, 0.0);
			for (int i = 0; i < n; ++i) {
				const double phi[4] = {1.0, u[i], std::sin(w * u[i]), std::cos(w * u[i])};
				for (int r = 0; r < 4; ++r) {
					for (int c = 0; c < 4; ++c) {
						a[r][c] += phi[r] * phi[c];
					}
					b[r] += phi[r] * y[i];
				}
			}
			QVector<QVector<double>> aSolve = a;
			QVector<double> coeff = b;
			if (!solveLinearSystem(aSolve, coeff)) {
				continue;
			}
			double rss = 0.0;
			for (int i = 0; i < n; ++i) {
				const double model = coeff[0] + coeff[1] * u[i] +
				                     coeff[2] * std::sin(w * u[i]) + coeff[3] * std::cos(w * u[i]);
				const double e = y[i] - model;
				rss += e * e;
			}
			if (bestRss < 0.0 || rss < bestRss) {
				bestRss = rss;
				bestC0 = coeff[0];
				bestC1 = coeff[1];
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

int PECurve::autoSmoothWindow(int sampleCount) {
	return std::max(3, 2 * (sampleCount / 50) + 1);
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
	int nfft = nextPow2(n0);
	while (nfft < n0 * 4 && nfft < (1 << 17)) {
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

	// The fundamental is the strongest peak at or above minFrequencyHz (i.e.
	// within maxPeriodS), so long-period drift leakage near DC can't hijack it
	// and turn the real periodic error into some huge-numbered "harmonic".
	int fundIdx = -1;
	for (int i = 0; i < rawPeaks.size(); i++) {
		if (rawPeaks.at(i).frequencyHz < minFrequencyHz) {
			continue;
		}
		if (fundIdx < 0 || rawPeaks.at(i).amplitude > rawPeaks.at(fundIdx).amplitude) {
			fundIdx = i;
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
	fundamental.relativeAmplitude = 1.0;
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
		const double relAmp = rp.amplitude / fund.amplitude;
		if (relAmp < minRelativeAmplitude) {
			continue;
		}
		PEFFTPeak p;
		p.harmonic = std::max(2, static_cast<int>(std::lround(rp.frequencyHz / fund.frequencyHz)));
		p.frequencyHz = rp.frequencyHz;
		p.periodS = 1.0 / rp.frequencyHz;
		p.amplitude = rp.amplitude;
		p.relativeAmplitude = relAmp;
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
