// Copyright (c) 2026
// All rights reserved.

#include "guidelogstats.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

QString normalizedHeader(QString header) {
	header = header.trimmed().toLower();
	header.replace("\"", "");
	return header;
}

// Returns the index of the first header matching any of the candidate names,
// comparing case-insensitively and ignoring quoting.
int findColumn(const QStringList &headers, const QStringList &names) {
	for (const QString &name : names) {
		const QString wanted = normalizedHeader(name);
		for (int i = 0; i < headers.size(); ++i) {
			if (normalizedHeader(headers.at(i)) == wanted) {
				return i;
			}
		}
	}
	return -1;
}

// Reads an (RA, Dec) pair from a row. Returns false unless both parse cleanly.
bool readPair(const QStringList &row, int raColumn, int decColumn, double &ra, double &dec) {
	if (raColumn < 0 || decColumn < 0) {
		return false;
	}
	bool raOk = false;
	bool decOk = false;
	ra = row.at(raColumn).toDouble(&raOk);
	dec = row.at(decColumn).toDouble(&decOk);
	return raOk && decOk;
}

// Accumulates one sample. raRmse/decRmse/combinedRmse hold running sums of
// squares until finalizeStats() converts them into RMSE values.
void accumulateSample(GuideAxisStats &stats, double ra, double dec) {
	const double combined = std::sqrt(ra * ra + dec * dec);
	stats.count++;
	stats.raRmse += ra * ra;
	stats.decRmse += dec * dec;
	stats.combinedRmse += combined * combined;
	stats.raPeak = std::max(stats.raPeak, std::abs(ra));
	stats.decPeak = std::max(stats.decPeak, std::abs(dec));
	stats.combinedPeak = std::max(stats.combinedPeak, combined);
}

void finalizeStats(GuideAxisStats &stats) {
	if (stats.count <= 0) {
		return;
	}
	const double count = static_cast<double>(stats.count);
	stats.raRmse = std::sqrt(stats.raRmse / count);
	stats.decRmse = std::sqrt(stats.decRmse / count);
	stats.combinedRmse = std::sqrt(stats.combinedRmse / count);
}

// Minimum residual samples before the lag-1 estimate is trustworthy.
const int kMinBalanceSamples = 12;

// Winsorising threshold for the balance estimate: deviations beyond this many
// robust sigmas (MAD-based) are clamped so a single spike can't dominate the
// correlation sums and flip the sign of ρ₁. Tunable; ~4 clips only genuine
// outliers while leaving normal loop dynamics untouched.
const double kBalanceOutlierSigma = 4.0;

// Median of v via nth_element (O(n)); reorders v in place.
double medianInPlace(std::vector<double> &v) {
	const int n = static_cast<int>(v.size());
	const int mid = n / 2;
	std::nth_element(v.begin(), v.begin() + mid, v.end());
	const double hi = v[mid];
	if (n & 1) {
		return hi;
	}
	// Even count: average the two central order statistics. Everything left of
	// mid is already <= hi after nth_element, so the lower median is their max.
	const double lo = *std::max_element(v.begin(), v.begin() + mid);
	return 0.5 * (lo + hi);
}

// Lag-1 autocorrelation of a residual series, made robust against spikes:
// median-centred (resists a shifted baseline) with each deviation winsorised to
// ±kBalanceOutlierSigma robust sigmas so a lone outlier cannot flip the result.
// Returns 0 for a flat or too-short series; ok reports whether the estimate is
// meaningful. O(n): median + MAD via nth_element, then one correlation pass.
double lag1Autocorrelation(const QVector<double> &series, bool *ok) {
	const int n = series.size();
	if (n < kMinBalanceSamples) {
		if (ok) {
			*ok = false;
		}
		return 0.0;
	}

	// Robust centre: the median resists spikes and baseline offset far better
	// than the mean. Work on a scratch copy so nth_element can reorder freely.
	std::vector<double> scratch(series.cbegin(), series.cend());
	const double centre = medianInPlace(scratch);

	// Robust scale: MAD (median absolute deviation) scaled to a sigma-equivalent.
	// Reuse the scratch buffer to hold |x - centre|.
	for (int i = 0; i < n; ++i) {
		scratch[i] = std::fabs(series.at(i) - centre);
	}
	const double scale = 1.4826 * medianInPlace(scratch); // ~std dev for normal data

	// Winsorise deviations to ±cap. When MAD ~ 0 (constant/quantised series) skip
	// capping so we don't clamp every deviation to zero.
	const bool capEnabled = scale > 1e-12;
	const double cap = kBalanceOutlierSigma * scale;
	auto centred = [&](int i) {
		const double d = series.at(i) - centre;
		if (!capEnabled) {
			return d;
		}
		return d > cap ? cap : (d < -cap ? -cap : d);
	};

	// Single O(n) pass over the winsorised, median-centred deviations.
	double numerator = 0.0;
	double denominator = 0.0;
	double prev = centred(0);
	denominator += prev * prev;
	for (int i = 1; i < n; ++i) {
		const double d = centred(i);
		denominator += d * d;
		numerator += d * prev;
		prev = d;
	}
	if (denominator < 1e-12) {
		if (ok) {
			*ok = false; // no variation — loop diagnostic is undefined
		}
		return 0.0;
	}
	if (ok) {
		*ok = true;
	}
	return numerator / denominator;
}

} // namespace

GuideColumns::GuideColumns(const QStringList &headers) {
	timestamp = headers.indexOf("Timestamp");
	raPixel = findColumn(headers, {"RA Dif", "RA Dif(\")"});
	decPixel = findColumn(headers, {"Dec Dif", "Dec Dif(\")"});
	raArc = findColumn(headers, {"RA Dif(\")"});
	decArc = findColumn(headers, {"Dec Dif(\")"});
	raCorr = findColumn(headers, {"Ra Correction", "RA Corr", "RA Correction"});
	dither = findColumn(headers, {"Dithering", "Dither"});
}

GuideRowSelection GuideLogStats::filterRows(const QVector<QStringList> &rows,
                                            const GuideColumns &columns,
                                            const GuideRowFilter &filter) {
	GuideRowSelection selection;
	selection.visibleRows.reserve(rows.size());

	for (int row = 0; row < rows.size(); ++row) {
		if (filter.useWindow && (row < filter.windowStart || row > filter.windowEnd)) {
			continue;
		}
		if (filter.selectionOnly && !filter.selectedRows.contains(row)) {
			continue;
		}
		if (filter.excludeDither && columns.dither >= 0) {
			bool ditherOk = false;
			const double ditherValue = rows.at(row).at(columns.dither).toDouble(&ditherOk);
			if (ditherOk && ditherValue != 0.0) {
				selection.ditherRowsExcluded++;
				continue;
			}
		}
		selection.visibleRows.append(row);
	}

	return selection;
}

GuideRowSelection GuideLogStats::excludeDitherRows(const QVector<QStringList> &rows,
                                                   const QVector<int> &inputRows,
                                                   const GuideColumns &columns) {
	GuideRowSelection selection;
	selection.visibleRows.reserve(inputRows.size());

	for (int row : inputRows) {
		if (columns.dither >= 0) {
			bool ditherOk = false;
			const double ditherValue = rows.at(row).at(columns.dither).toDouble(&ditherOk);
			if (ditherOk && ditherValue != 0.0) {
				selection.ditherRowsExcluded++;
				continue;
			}
		}
		selection.visibleRows.append(row);
	}

	return selection;
}

GuideStatsResult GuideLogStats::compute(const QVector<QStringList> &rows,
                                        const QVector<int> &visibleRows,
                                        const GuideColumns &columns,
                                        int totalRows,
                                        int ditherRowsExcluded) {
	GuideStatsResult result;
	result.rowsShown = visibleRows.size();
	result.totalRows = totalRows;
	result.ditherRowsExcluded = ditherRowsExcluded;

	// Residual series (in row order) for the over-/under-correction diagnostic.
	QVector<double> raSeries;
	QVector<double> decSeries;
	raSeries.reserve(visibleRows.size());
	decSeries.reserve(visibleRows.size());

	for (int row : visibleRows) {
		const QStringList &values = rows.at(row);
		double ra = 0.0;
		double dec = 0.0;
		if (readPair(values, columns.raPixel, columns.decPixel, ra, dec)) {
			accumulateSample(result.pixels, ra, dec);
		}
		if (readPair(values, columns.raArc, columns.decArc, ra, dec)) {
			accumulateSample(result.arcsec, ra, dec);
		}
		if (columns.raPixel >= 0) {
			bool ok = false;
			const double v = values.at(columns.raPixel).toDouble(&ok);
			if (ok) {
				raSeries.append(v);
			}
		}
		if (columns.decPixel >= 0) {
			bool ok = false;
			const double v = values.at(columns.decPixel).toDouble(&ok);
			if (ok) {
				decSeries.append(v);
			}
		}
	}

	result.balance.raLag1 = lag1Autocorrelation(raSeries, &result.balance.raValid);
	result.balance.decLag1 = lag1Autocorrelation(decSeries, &result.balance.decValid);

	finalizeStats(result.pixels);
	finalizeStats(result.arcsec);
	return result;
}
