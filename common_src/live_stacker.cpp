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

#include "live_stacker.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

// ---------------------------------------------------------------------------
// Alignment parameters — NCC
// ---------------------------------------------------------------------------

// Downsample factor for the coarse cross-correlation pass.
// A 4000x3000 frame → 250x187 at DS=16.  Fast to correlate.
static const int DS_COARSE = 16;

// Downsample factor for the fine cross-correlation pass.
static const int DS_FINE = 4;

// Maximum translation to search for, in ORIGINAL pixel coordinates.
static const int SEARCH_PX = 512;

// ---------------------------------------------------------------------------
// Alignment parameters — centroid-based
// ---------------------------------------------------------------------------

// Downsample factor used when building the luminance map for star detection.
// DS=2 gives a good balance: fast, yet accurate sub-pixel centroids.
static const int DS_STAR = 2;

// A pixel is a star candidate if its value (after mean subtraction) exceeds
// this many standard deviations above the background noise.
static const double STAR_SIGMA = 5.0;

// Non-maximum suppression radius (downsampled pixels).
// Prevents the same star from being detected multiple times.
static const int STAR_NMS_RADIUS = 3;

// Half-size of the intensity-weighted centroid window (downsampled pixels).
// The window is (2*STAR_WIN_HALF+1)^2, e.g. 9×9 for STAR_WIN_HALF=4.
static const int STAR_WIN_HALF = 4;

// Maximum number of the brightest stars to retain per frame.
static const int STAR_MAX_COUNT = 100;

// A reference star and a current-frame star are considered matched when
// their distance in ORIGINAL pixel coordinates is below this value.
static const float STAR_MATCH_RADIUS = 50.0f;

// Minimum number of matched star pairs required for a valid centroid shift.
// If fewer pairs are found the method falls back to NCC.
static const int STAR_MIN_MATCHES = 3;

// ---------------------------------------------------------------------------
// Format helpers
// ---------------------------------------------------------------------------

static int channelsForFormat(int pix_format) {
	switch (pix_format) {
		case PIX_FMT_Y8:
		case PIX_FMT_Y16:
		case PIX_FMT_F32:   return 1;
		case PIX_FMT_RGB24:
		case PIX_FMT_RGB48:
		case PIX_FMT_RGBF:  return 3;
		default:            return 0;
	}
}

static int bytesPerSample(int pix_format) {
	switch (pix_format) {
		case PIX_FMT_Y8:    return 1;
		case PIX_FMT_Y16:   return 2;
		case PIX_FMT_F32:   return 4;
		case PIX_FMT_RGB24: return 1;
		case PIX_FMT_RGB48: return 2;
		case PIX_FMT_RGBF:  return 4;
		default:            return 0;
	}
}

// ---------------------------------------------------------------------------
// LiveStacker
// ---------------------------------------------------------------------------

LiveStacker::LiveStacker()
	: m_alignment_method(ALIGN_CENTROIDS)
	, m_width(0)
	, m_height(0)
	, m_channels(0)
	, m_pix_format(0)
	, m_frame_count(0)
{}

void LiveStacker::startStack() { resetStack(); }

void LiveStacker::resetStack() {
	m_acc.clear();
	m_ref_coarse.clear();
	m_ref_fine.clear();
	m_ref_stars.clear();
	m_width       = 0;
	m_height      = 0;
	m_channels    = 0;
	m_pix_format  = 0;
	m_frame_count = 0;
}

// ---------------------------------------------------------------------------
// buildLuminanceMap
//
// Box-average downsampled by factor @p ds, then subtract the mean.
// The DC-removed map makes NCC scores independent of sky background level.
// ---------------------------------------------------------------------------

std::vector<float> LiveStacker::buildLuminanceMap(preview_image *image, int ds) const {
	const int W  = image->m_width;
	const int H  = image->m_height;
	const int dW = W / ds;
	const int dH = H / ds;
	const char *raw = image->m_raw_data;

	std::vector<float> lum(static_cast<size_t>(dW) * dH, 0.0f);

	for (int by = 0; by < dH; ++by) {
		for (int bx = 0; bx < dW; ++bx) {
			double sum = 0.0;
			int cnt = 0;
			for (int dy = 0; dy < ds; ++dy) {
				int y = by * ds + dy;
				if (y >= H) continue;
				for (int dx = 0; dx < ds; ++dx) {
					int x = bx * ds + dx;
					if (x >= W) continue;
					int idx = y * W + x;
					double val;
					if (m_channels == 1) {
						switch (m_pix_format) {
							case PIX_FMT_Y8:  val = reinterpret_cast<const uint8_t  *>(raw)[idx]; break;
							case PIX_FMT_Y16: val = reinterpret_cast<const uint16_t *>(raw)[idx]; break;
							default:          val = reinterpret_cast<const float    *>(raw)[idx]; break;
						}
					} else {
						int base = idx * 3;
						double r, g, b;
						switch (m_pix_format) {
							case PIX_FMT_RGB24: { const auto *p = reinterpret_cast<const uint8_t  *>(raw); r=p[base]; g=p[base+1]; b=p[base+2]; break; }
							case PIX_FMT_RGB48: { const auto *p = reinterpret_cast<const uint16_t *>(raw); r=p[base]; g=p[base+1]; b=p[base+2]; break; }
							default:            { const auto *p = reinterpret_cast<const float    *>(raw); r=p[base]; g=p[base+1]; b=p[base+2]; break; }
						}
						val = 0.2126*r + 0.7152*g + 0.0722*b;
					}
					sum += val;
					++cnt;
				}
			}
			lum[by * dW + bx] = (cnt > 0) ? static_cast<float>(sum / cnt) : 0.0f;
		}
	}

	// Subtract mean (removes sky background bias)
	double mean = 0.0;
	for (float v : lum) mean += v;
	mean /= static_cast<double>(lum.size());
	for (float &v : lum) v -= static_cast<float>(mean);

	return lum;
}

// ---------------------------------------------------------------------------
// nccScore
//
// Normalised cross-correlation between ref and cur at integer shift (dx, dy)
// (in the downsampled coordinate system).  Returns a value in [−1, 1];
// higher = better alignment.  Only considers the overlapping region.
// ---------------------------------------------------------------------------

double LiveStacker::nccScore(const std::vector<float> &ref,
                              const std::vector<float> &cur,
                              int dW, int dH, int dx, int dy) {
	int rx_start = std::max(0, -dx);
	int rx_end   = std::min(dW, dW - dx);
	int ry_start = std::max(0, -dy);
	int ry_end   = std::min(dH, dH - dy);

	if (rx_end <= rx_start || ry_end <= ry_start) return -1.0;

	double dot = 0.0, refNorm = 0.0, curNorm = 0.0;
	for (int ry = ry_start; ry < ry_end; ++ry) {
		int sy = ry + dy;
		const float *rRow = ref.data() + ry * dW + rx_start;
		const float *cRow = cur.data() + sy * dW + rx_start + dx;
		int count = rx_end - rx_start;
		for (int i = 0; i < count; ++i) {
			double rv = rRow[i];
			double cv = cRow[i];
			dot     += rv * cv;
			refNorm += rv * rv;
			curNorm += cv * cv;
		}
	}

	double denom = std::sqrt(refNorm * curNorm);
	return (denom > 1e-12) ? dot / denom : -1.0;
}

// ---------------------------------------------------------------------------
// findShift  (2-level pyramid)
//
// Level 1 – coarse (DS_COARSE):
//   Search ±SEARCH_PX original pixels → ±(SEARCH_PX/DS_COARSE) downsampled.
//   E.g. 512/32 = 16 → 33×33 = 1089 evaluations on a ~125×93 map → ~12 M ops.
//
// Level 2 – fine (DS_FINE):
//   Refine ±(DS_COARSE/DS_FINE) downsampled pixels around the coarse result.
//   E.g. ±(32/4) = ±8 → 17×17 = 289 evaluations on a ~1000×750 map → ~217 M ops.
//   At ~4 GFLOP/s scalar: < 100 ms per frame.
//
// Parabolic sub-pixel refinement applied to the fine result.
// ---------------------------------------------------------------------------

void LiveStacker::findShift(int &shift_x, int &shift_y,
                             const std::vector<float> &cur_coarse,
                             const std::vector<float> &cur_fine) const {
	// --- Level 1: coarse search ---
	const int dW_c = m_width  / DS_COARSE;
	const int dH_c = m_height / DS_COARSE;
	const int srC  = SEARCH_PX / DS_COARSE;

	double bestC = -2.0;
	int bestDxC = 0, bestDyC = 0;
	for (int tdy = -srC; tdy <= srC; ++tdy) {
		for (int tdx = -srC; tdx <= srC; ++tdx) {
			double s = nccScore(m_ref_coarse, cur_coarse, dW_c, dH_c, tdx, tdy);
			if (s > bestC) { bestC = s; bestDxC = tdx; bestDyC = tdy; }
		}
	}

	// Convert coarse result to original pixel coordinates
	int coarse_ox = bestDxC * DS_COARSE;
	int coarse_oy = bestDyC * DS_COARSE;

	// --- Level 2: fine search around coarse result ---
	const int dW_f = m_width  / DS_FINE;
	const int dH_f = m_height / DS_FINE;
	// In downsampled-fine space, the coarse result maps to:
	int centre_fx = coarse_ox / DS_FINE;
	int centre_fy = coarse_oy / DS_FINE;
	// Search ±(DS_COARSE/DS_FINE) = ±8 fine pixels around it
	const int srF = DS_COARSE / DS_FINE + 1;  // a little extra margin

	double bestF = -2.0;
	int bestDxF = centre_fx, bestDyF = centre_fy;
	for (int tdy = centre_fy - srF; tdy <= centre_fy + srF; ++tdy) {
		for (int tdx = centre_fx - srF; tdx <= centre_fx + srF; ++tdx) {
			double s = nccScore(m_ref_fine, cur_fine, dW_f, dH_f, tdx, tdy);
			if (s > bestF) { bestF = s; bestDxF = tdx; bestDyF = tdy; }
		}
	}

	// --- Parabolic sub-pixel refinement of fine result ---
	double subDxF = bestDxF, subDyF = bestDyF;

	auto scoreF = [&](int ddx, int ddy) {
		return nccScore(m_ref_fine, cur_fine, dW_f, dH_f, ddx, ddy);
	};

	if (bestDxF > -(dW_f-1) && bestDxF < (dW_f-1)) {
		double f_m = scoreF(bestDxF - 1, bestDyF);
		double f_0 = bestF;
		double f_p = scoreF(bestDxF + 1, bestDyF);
		double denom = 2.0*(f_m - 2.0*f_0 + f_p);
		if (std::abs(denom) > 1e-12)
			subDxF = bestDxF - (f_p - f_m) / denom;
	}
	if (bestDyF > -(dH_f-1) && bestDyF < (dH_f-1)) {
		double f_m = scoreF(bestDxF, bestDyF - 1);
		double f_0 = bestF;
		double f_p = scoreF(bestDxF, bestDyF + 1);
		double denom = 2.0*(f_m - 2.0*f_0 + f_p);
		if (std::abs(denom) > 1e-12)
			subDyF = bestDyF - (f_p - f_m) / denom;
	}

	shift_x = static_cast<int>(std::round(subDxF * DS_FINE));
	shift_y = static_cast<int>(std::round(subDyF * DS_FINE));
}

// ---------------------------------------------------------------------------
// accumulate
// ---------------------------------------------------------------------------

void LiveStacker::accumulate(preview_image *image, int dx, int dy) {
	const char *raw = image->m_raw_data;
	const int W = m_width;
	const int H = m_height;

	for (int y = 0; y < H; ++y) {
		int sy = y + dy;
		if (sy < 0 || sy >= H) continue;
		for (int x = 0; x < W; ++x) {
			int sx = x + dx;
			if (sx < 0 || sx >= W) continue;

			int dst_idx = y * W + x;
			int src_idx = sy * W + sx;

			if (m_channels == 1) {
				double val;
				switch (m_pix_format) {
					case PIX_FMT_Y8:  val = reinterpret_cast<const uint8_t  *>(raw)[src_idx]; break;
					case PIX_FMT_Y16: val = reinterpret_cast<const uint16_t *>(raw)[src_idx]; break;
					default:          val = reinterpret_cast<const float    *>(raw)[src_idx]; break;
				}
				m_acc[dst_idx] += val;
			} else {
				int base = src_idx * 3;
				double r, g, b;
				switch (m_pix_format) {
					case PIX_FMT_RGB24: { const auto *p = reinterpret_cast<const uint8_t  *>(raw); r=p[base]; g=p[base+1]; b=p[base+2]; break; }
					case PIX_FMT_RGB48: { const auto *p = reinterpret_cast<const uint16_t *>(raw); r=p[base]; g=p[base+1]; b=p[base+2]; break; }
					default:            { const auto *p = reinterpret_cast<const float    *>(raw); r=p[base]; g=p[base+1]; b=p[base+2]; break; }
				}
				m_acc[dst_idx * 3    ] += r;
				m_acc[dst_idx * 3 + 1] += g;
				m_acc[dst_idx * 3 + 2] += b;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// detectStars
//
// Produces a list of up to STAR_MAX_COUNT star centroids detected in @p image.
//
// Algorithm:
//  1. Build a mean-subtracted, box-downsampled (DS_STAR) luminance map so that
//     background pixels gather around 0 and star cores are large positive peaks.
//  2. Estimate noise sigma as the RMS of the mean-subtracted values.
//  3. Scan for local maxima inside a (2*STAR_NMS_RADIUS+1)^2 window that exceed
//     STAR_SIGMA * sigma.  Each qualifying pixel is a star candidate.
//  4. Refine each candidate with an intensity-weighted centroid over a
//     (2*STAR_WIN_HALF+1)^2 window, clipping negative values to 0 so only
//     the star flux contributes.
//  5. Sort by integrated flux, keep the top STAR_MAX_COUNT stars, and
//     scale coordinates back to original-pixel space.
// ---------------------------------------------------------------------------

std::vector<StarCentroid> LiveStacker::detectStars(preview_image *image) const {
	const int ds = DS_STAR;
	const int dW = image->m_width  / ds;
	const int dH = image->m_height / ds;
	if (dW < 4 || dH < 4) return {};

	// Mean-subtracted luminance: background ≈ 0, stars > 0.
	std::vector<float> lum = buildLuminanceMap(image, ds);

	// Noise sigma: RMS of all pixels (mean is already 0 after buildLuminanceMap).
	double var = 0.0;
	for (float v : lum) var += static_cast<double>(v) * v;
	var /= static_cast<double>(lum.size());
	const double sigma = std::sqrt(var);
	const float threshold = static_cast<float>(STAR_SIGMA * sigma);
	if (threshold <= 0.0f) return {};

	const int nr = STAR_NMS_RADIUS;
	const int wh = STAR_WIN_HALF;
	std::vector<StarCentroid> candidates;
	candidates.reserve(256);

	for (int y = nr; y < dH - nr; ++y) {
		for (int x = nr; x < dW - nr; ++x) {
			float v = lum[y * dW + x];
			if (v < threshold) continue;

			// Non-maximum suppression: must be the brightest in the NMS window.
			bool isMax = true;
			for (int dy = -nr; dy <= nr && isMax; ++dy) {
				for (int dx = -nr; dx <= nr && isMax; ++dx) {
					if (dx == 0 && dy == 0) continue;
					if (lum[(y + dy) * dW + (x + dx)] >= v) isMax = false;
				}
			}
			if (!isMax) continue;

			// Intensity-weighted centroid within the refinement window.
			double cx   = 0.0, cy = 0.0, flux = 0.0;
			const int x0 = std::max(0, x - wh);
			const int x1 = std::min(dW - 1, x + wh);
			const int y0 = std::max(0, y - wh);
			const int y1 = std::min(dH - 1, y + wh);
			for (int py = y0; py <= y1; ++py) {
				for (int px = x0; px <= x1; ++px) {
					float w = std::max(0.0f, lum[py * dW + px]);
					cx   += w * px;
					cy   += w * py;
					flux += w;
				}
			}
			if (flux < 1e-6f) continue;
			cx /= flux;
			cy /= flux;

			StarCentroid sc;
			sc.x    = static_cast<float>(cx * ds); // back to original coords
			sc.y    = static_cast<float>(cy * ds);
			sc.flux = static_cast<float>(flux);
			candidates.push_back(sc);
		}
	}

	// Keep only the STAR_MAX_COUNT brightest stars.
	std::sort(candidates.begin(), candidates.end(),
	          [](const StarCentroid &a, const StarCentroid &b){ return a.flux > b.flux; });
	if (static_cast<int>(candidates.size()) > STAR_MAX_COUNT)
		candidates.resize(STAR_MAX_COUNT);

	return candidates;
}

// ---------------------------------------------------------------------------
// findShiftByCentroids
//
// For each star in m_ref_stars, find the nearest star in cur_stars within
// STAR_MATCH_RADIUS original pixels.  The shift is the median (dx, dy) over
// all matched pairs.  Returns false when fewer than STAR_MIN_MATCHES pairs
// are found (caller should fall back to NCC).
// ---------------------------------------------------------------------------

bool LiveStacker::findShiftByCentroids(int &shift_x, int &shift_y,
                                        const std::vector<StarCentroid> &cur_stars) const {
	if (m_ref_stars.empty() || cur_stars.empty()) return false;

	const float matchR2 = STAR_MATCH_RADIUS * STAR_MATCH_RADIUS;
	std::vector<float> dxs, dys;
	dxs.reserve(m_ref_stars.size());
	dys.reserve(m_ref_stars.size());

	for (const StarCentroid &ref : m_ref_stars) {
		float bestDist2 = matchR2 + 1.0f;
		float bestDx = 0.0f, bestDy = 0.0f;
		bool found = false;

		for (const StarCentroid &cur : cur_stars) {
			float ddx = cur.x - ref.x;
			float ddy = cur.y - ref.y;
			float d2  = ddx * ddx + ddy * ddy;
			if (d2 < bestDist2) {
				bestDist2 = d2;
				bestDx    = ddx;
				bestDy    = ddy;
				found     = true;
			}
		}
		if (found) {
			dxs.push_back(bestDx);
			dys.push_back(bestDy);
		}
	}

	if (static_cast<int>(dxs.size()) < STAR_MIN_MATCHES) {
		shift_x = shift_y = 0;
		return false;
	}

	// Median shift — robust against a minority of bad matches.
	const size_t mid = dxs.size() / 2;
	std::nth_element(dxs.begin(), dxs.begin() + mid, dxs.end());
	std::nth_element(dys.begin(), dys.begin() + mid, dys.end());

	shift_x = static_cast<int>(std::round(dxs[mid]));
	shift_y = static_cast<int>(std::round(dys[mid]));
	return true;
}

// ---------------------------------------------------------------------------
// addImage
// ---------------------------------------------------------------------------

bool LiveStacker::addImage(preview_image *image) {
	if (!image || !image->m_raw_data)
		return false;

	const int fmt = image->m_pix_format;
	const int ch  = channelsForFormat(fmt);
	if (ch == 0 || bytesPerSample(fmt) == 0)
		return false;

	const int W = image->m_width;
	const int H = image->m_height;

	if (m_frame_count == 0) {
		m_width      = W;
		m_height     = H;
		m_channels   = ch;
		m_pix_format = fmt;
		m_acc.assign(static_cast<size_t>(W) * H * ch, 0.0);

		// NCC reference maps — always built so the fallback path always works.
		m_ref_coarse = buildLuminanceMap(image, DS_COARSE);
		m_ref_fine   = buildLuminanceMap(image, DS_FINE);

		// Centroid reference stars — built when ALIGN_CENTROIDS is selected.
		if (m_alignment_method == ALIGN_CENTROIDS)
			m_ref_stars = detectStars(image);

		accumulate(image, 0, 0);
	} else {
		if (W != m_width || H != m_height || fmt != m_pix_format)
			return false;

		int dx = 0, dy = 0;

		bool aligned = false;
		if (m_alignment_method == ALIGN_CENTROIDS && !m_ref_stars.empty()) {
			std::vector<StarCentroid> cur_stars = detectStars(image);
			aligned = findShiftByCentroids(dx, dy, cur_stars);
		}

		// Fallback to NCC when centroid method is not selected or failed.
		if (!aligned) {
			std::vector<float> cur_coarse = buildLuminanceMap(image, DS_COARSE);
			std::vector<float> cur_fine   = buildLuminanceMap(image, DS_FINE);
			findShift(dx, dy, cur_coarse, cur_fine);
		}

		accumulate(image, dx, dy);
	}

	++m_frame_count;
	return true;
}

// ---------------------------------------------------------------------------
// currentStack
// ---------------------------------------------------------------------------

preview_image *LiveStacker::currentStack() const {
	if (m_frame_count == 0)
		return nullptr;

	const int samples = m_width * m_height * m_channels;

	const size_t byte_size = static_cast<size_t>(samples) * sizeof(float);
	auto owner = std::shared_ptr<char>(new char[byte_size], std::default_delete<char[]>());
	float *dst = reinterpret_cast<float *>(owner.get());

	const double inv = 1.0 / m_frame_count;
	for (int i = 0; i < samples; ++i)
		dst[i] = static_cast<float>(m_acc[i] * inv);

	const int out_fmt = (m_channels == 1) ? PIX_FMT_F32 : PIX_FMT_RGBF;
	stretch_config_t sconfig{};
	return create_preview(m_width, m_height, out_fmt, owner, owner.get(), sconfig);
}
