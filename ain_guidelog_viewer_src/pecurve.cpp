// Copyright (c) 2026
// All rights reserved.

#include "pecurve.h"

#include <QDateTime>

#include <algorithm>
#include <cmath>

#include "guidelogstats.h"

namespace {

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
