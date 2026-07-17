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

#ifndef GUIDELOGSTATS_H
#define GUIDELOGSTATS_H

#include <QStringList>
#include <QVector>
#include <QSet>

// Per-axis RMSE / peak statistics for one unit system (pixels or arcsec).
struct GuideAxisStats {
	int count = 0;
	double raRmse = 0.0;
	double decRmse = 0.0;
	double combinedRmse = 0.0;
	double raPeak = 0.0;
	double decPeak = 0.0;
	double combinedPeak = 0.0;

	bool isValid() const { return count > 0; }
};

// Resolves the meaningful guiding columns for a given header row. Each index is
// -1 when the corresponding header is absent.
struct GuideColumns {
	GuideColumns() = default;
	explicit GuideColumns(const QStringList &headers);

	int timestamp = -1;
	int raPixel = -1;
	int decPixel = -1;
	int raArc = -1;
	int decArc = -1;
	int raCorr = -1;
	int dither = -1;
};

// UI-independent description of which rows should be taken into account.
struct GuideRowFilter {
	bool useWindow = false;
	int windowStart = 0;
	int windowEnd = 0;
	bool selectionOnly = false;
	QSet<int> selectedRows;
	bool excludeDither = false;
};

// Result of applying a GuideRowFilter to a set of rows.
struct GuideRowSelection {
	QVector<int> visibleRows;
	int ditherRowsExcluded = 0;
};

// Over-/under-correction diagnostic per axis. It is the lag-1 autocorrelation of
// the residual error: ~0 means the loop is well tuned (white residual), > 0 means
// the error persists frame-to-frame (under-correcting, aggressiveness too low),
// and < 0 means it flips sign frame-to-frame (over-correcting, the loop rings).
struct CorrectionBalance {
	bool raValid = false;
	bool decValid = false;
	double raLag1 = 0.0;
	double decLag1 = 0.0;
};

// Aggregated statistics produced from a set of visible rows.
struct GuideStatsResult {
	GuideAxisStats pixels;
	GuideAxisStats arcsec;
	CorrectionBalance balance;
	int rowsShown = 0;
	int totalRows = 0;
	int ditherRowsExcluded = 0;
};

// Pure statistics calculator: keeps all guiding math out of the window/UI code.
class GuideLogStats {
public:
	// Returns the rows that pass the filter, in ascending order, together with
	// the number of dithering rows that were excluded.
	static GuideRowSelection filterRows(const QVector<QStringList> &rows,
	                                     const GuideColumns &columns,
	                                     const GuideRowFilter &filter);

	// Drops dithering rows (Dithering != 0) from the given row list, returning
	// the survivors and how many were removed. Lets the graph keep dither frames
	// while the statistics leave them out.
	static GuideRowSelection excludeDitherRows(const QVector<QStringList> &rows,
	                                            const QVector<int> &inputRows,
	                                            const GuideColumns &columns);

	// Computes RMSE / peak statistics over the given visible rows.
	static GuideStatsResult compute(const QVector<QStringList> &rows,
	                                 const QVector<int> &visibleRows,
	                                 const GuideColumns &columns,
	                                 int totalRows,
	                                 int ditherRowsExcluded);
};

#endif // GUIDELOGSTATS_H
