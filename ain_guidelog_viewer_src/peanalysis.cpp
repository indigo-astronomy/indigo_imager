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

#include "peanalysis.h"

void PEAnalysis::setRows(const QStringList &headers, const QVector<QStringList> &rows) {
	// The main window re-pushes the graph's visible rows on every interaction,
	// and most of those pushes carry exactly the same rows as the last one.
	// Comparing first turns those into a no-op and keeps the cache warm. Qt's
	// containers are implicitly shared, so an unchanged push compares by pointer
	// and costs nothing.
	if (m_hasRows && headers == m_headers && rows == m_rows) {
		return;
	}
	m_headers = headers;
	m_rows = rows;
	m_hasRows = true;
	m_samples = PESamples::fromRows(headers, rows);
	m_cache[0].reset();
	m_cache[1].reset();
}

std::shared_ptr<PEResult> PEAnalysis::cachedFor(const PECurveOptions &options) {
	for (int i = 0; i < 2; i++) {
		if (m_cache[i] && m_cache[i]->options == options) {
			if (i != 0) {
				m_cache[i].swap(m_cache[0]); // keep the most recent hit first
			}
			return m_cache[0];
		}
	}
	return nullptr;
}

void PEAnalysis::remember(const std::shared_ptr<PEResult> &result) {
	m_cache[1] = m_cache[0];
	m_cache[0] = result;
}

std::shared_ptr<const PEResult> PEAnalysis::reconstruct(const PECurveOptions &options) {
	if (const std::shared_ptr<PEResult> hit = cachedFor(options)) {
		return hit;
	}
	auto result = std::make_shared<PEResult>();
	result->options = options;
	result->curve = PECurve::reconstruct(m_samples, options);
	remember(result);
	return result;
}

std::shared_ptr<const PEResult> PEAnalysis::reconstructWithSpectrum(const PECurveOptions &options) {
	std::shared_ptr<PEResult> result = cachedFor(options);
	if (!result) {
		result = std::make_shared<PEResult>();
		result->options = options;
		result->curve = PECurve::reconstruct(m_samples, options);
		remember(result);
	}
	if (!result->spectrumComputed) {
		if (result->curve.valid) {
			result->spectrum = PECurve::computeFFT(result->curve.x, result->curve.pe);
		} else {
			result->spectrum = PEFFTData();
			result->spectrum.message = result->curve.message;
		}
		result->spectrumComputed = true;
	}
	return result;
}
