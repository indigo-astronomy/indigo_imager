// Copyright (c) 2026
// All rights reserved.

#include "guidelogstats.h"

#include <algorithm>
#include <cmath>

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

} // namespace

GuideColumns::GuideColumns(const QStringList &headers) {
	timestamp = headers.indexOf("Timestamp");
	raPixel = findColumn(headers, {"RA Dif", "RA Dif(\")"});
	decPixel = findColumn(headers, {"Dec Dif", "Dec Dif(\")"});
	raArc = findColumn(headers, {"RA Dif(\")"});
	decArc = findColumn(headers, {"Dec Dif(\")"});
	dither = findColumn(headers, {"Dithering"});
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
	}

	finalizeStats(result.pixels);
	finalizeStats(result.arcsec);
	return result;
}
