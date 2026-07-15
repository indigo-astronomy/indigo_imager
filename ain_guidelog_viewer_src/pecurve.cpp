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
	const double sign = options.invert ? -1.0 : 1.0;
	out.hasRate = (rate > 0.0);

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
			cumCorrPx += sign * corrSec * rate;
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

	out.valid = true;
	out.usedTime = useTime;
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
