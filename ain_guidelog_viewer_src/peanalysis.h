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

#ifndef PEANALYSIS_H
#define PEANALYSIS_H

#include <QStringList>
#include <QVector>

#include <memory>

#include "pecurve.h"

// One reconstruction together with whatever was derived from it. The spectrum
// is filled in lazily: the PE curve window never asks for it.
struct PEResult {
	PECurveOptions options;
	PECurveData curve;

	bool spectrumComputed = false;
	PEFFTData spectrum;
};

// The rows currently under analysis, plus a small cache of what has been
// computed from them.
//
// The two periodic-error windows look at the same rows, and the main window
// re-pushes those rows on every pan/zoom of the graph. Without a shared cache
// each window decodes the log's text and re-runs the reconstruction on its own,
// twice per interaction. PEAnalysis decodes the rows once (PESamples) and hands
// both windows the same PEResult whenever their options agree — which is the
// case until the user changes a control in the PE curve window, hence the
// two-entry cache rather than a single slot.
class PEAnalysis {
public:
	// Replaces the rows under analysis and drops every cached result. Cheap and
	// safe to call with unchanged rows: it compares first and does nothing.
	void setRows(const QStringList &headers, const QVector<QStringList> &rows);

	// True once setRows() has been given a session (even an unusable one).
	bool hasRows() const { return m_hasRows; }

	const PESamples &samples() const { return m_samples; }

	// The reconstruction for these options, computed on first request and kept.
	std::shared_ptr<const PEResult> reconstruct(const PECurveOptions &options);

	// As reconstruct(), and additionally guarantees result->spectrum is filled
	// in (result->spectrumComputed is then true).
	std::shared_ptr<const PEResult> reconstructWithSpectrum(const PECurveOptions &options);

private:
	std::shared_ptr<PEResult> cachedFor(const PECurveOptions &options);
	void remember(const std::shared_ptr<PEResult> &result);

	bool m_hasRows = false;
	QStringList m_headers;
	QVector<QStringList> m_rows;
	PESamples m_samples;

	// Most-recently-used first. Two entries cover the two windows.
	std::shared_ptr<PEResult> m_cache[2];
};

#endif // PEANALYSIS_H
