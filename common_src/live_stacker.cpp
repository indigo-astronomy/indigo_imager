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
#include <thread>
#include <future>
#include <utils.h>
#include <chrono>

// ---------------------------------------------------------------------------
// Alignment parameters
// ---------------------------------------------------------------------------

// Maximum translation to search for, in ORIGINAL pixel coordinates.
// Used by the Hough histogram alignment.
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
// If fewer pairs are found the method returns false (no alignment).
static const int STAR_MIN_MATCHES = 3;

// ---------------------------------------------------------------------------
// Alignment parameters — Hough translation histogram
// ---------------------------------------------------------------------------

// Bin size for the 2-D translation histogram, in ORIGINAL pixel coordinates.
// Smaller = finer resolution but larger memory; 4 px is a good compromise.
static const int HOUGH_BIN_PX = 4;

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
	: m_alignment_method(ALIGN_KD_TREE_ROTATION)
	, m_interp_method(INTERP_BICUBIC)
	, m_width(0)
	, m_height(0)
	, m_channels(0)
	, m_pix_format(0)
	, m_frame_count(0)
{}

void LiveStacker::startStack() {
	resetStack();
}

void LiveStacker::resetStack() {
	m_acc.clear();
	m_ref_stars.clear();
	m_width = 0;
	m_height = 0;
	m_channels = 0;
	m_pix_format = 0;
	m_frame_count = 0;
}

// ---------------------------------------------------------------------------
// buildLuminanceMap
//
// Box-average downsampled by factor @p ds, then subtract the mean.
// The DC-removed map is used by star detection to suppress sky background.
// ---------------------------------------------------------------------------

std::vector<float> LiveStacker::buildLuminanceMap(preview_image *image, int ds) const {
	const int W = image->m_width;
	const int H = image->m_height;
	const int dW = W / ds;
	const int dH = H / ds;
	const char *raw = image->m_raw_data;

	std::vector<float> lum(static_cast<size_t>(dW) * dH, 0.0f);

	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	std::vector<std::thread> threads;
	float *lumData = lum.data();

	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = static_cast<int>(std::ceil(dH / (double)num_threads));
		threads.emplace_back([=]() {
			const int start_by = chunk * rank;
			const int end_by = std::min(start_by + chunk, dH);
			for (int by = start_by; by < end_by; ++by) {
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
									case PIX_FMT_Y8:
										val = reinterpret_cast<const uint8_t*>(raw)[idx];
										break;
									case PIX_FMT_Y16:
										val = reinterpret_cast<const uint16_t*>(raw)[idx];
										break;
									default:
										val = reinterpret_cast<const float*>(raw)[idx];
										break;
								}
							} else {
								int base = idx * 3;
								double r, g, b;
								switch (m_pix_format) {
									case PIX_FMT_RGB24: {
										const auto *p = reinterpret_cast<const uint8_t *>(raw);
										r=p[base];
										g=p[base+1];
										b=p[base+2];
										break;
									}
									case PIX_FMT_RGB48: {
										const auto *p = reinterpret_cast<const uint16_t*>(raw);
										r=p[base];
										g=p[base+1];
										b=p[base+2];
										break;
									}
									default: {
										const auto *p = reinterpret_cast<const float*>(raw);
										r=p[base];
										g=p[base+1];
										b=p[base+2];
										break;
									}
								}
								val = 0.2126*r + 0.7152*g + 0.0722*b;
							}
							sum += val;
							++cnt;
						}
					}
					lumData[by * dW + bx] = (cnt > 0) ? static_cast<float>(sum / cnt) : 0.0f;
				}
			}
		});
	}
	for (auto &t : threads) t.join();

	double mean = 0.0;
	const size_t total = static_cast<size_t>(dW) * dH;
	for (size_t i = 0; i < total; ++i) {
		mean += lumData[i];
	}
	mean /= static_cast<double>(total);

	threads.clear();
	for (int rank = 0; rank < num_threads; rank++) {
		const size_t chunk = static_cast<size_t>(std::ceil(total / (double)num_threads));
		threads.emplace_back([=]() {
			const size_t start = chunk * rank;
			const size_t end = std::min(start + chunk, total);
			for (size_t i = start; i < end; ++i) {
				lumData[i] = static_cast<float>(lumData[i] - mean);
			}
		});
	}
	for (auto &t : threads) t.join();

	return lum;
}

static inline double readSample(const char *raw, int W, int H, int pix_fmt, int sx, int sy, int ch) {
	sx = std::max(0, std::min(W - 1, sx));
	sy = std::max(0, std::min(H - 1, sy));
	const int idx = sy * W + sx;
	switch (pix_fmt) {
		case PIX_FMT_Y8:
			return static_cast<double>(reinterpret_cast<const uint8_t*>(raw)[idx]);
		case PIX_FMT_Y16:
			return static_cast<double>(reinterpret_cast<const uint16_t*>(raw)[idx]);
		case PIX_FMT_F32:
			return static_cast<double>(reinterpret_cast<const float*>(raw)[idx]);
		case PIX_FMT_RGB24:
			return static_cast<double>(reinterpret_cast<const uint8_t*>(raw)[idx * 3 + ch]);
		case PIX_FMT_RGB48:
			return static_cast<double>(reinterpret_cast<const uint16_t*>(raw)[idx * 3 + ch]);
		default:
			return static_cast<double>(reinterpret_cast<const float*>(raw)[idx * 3 + ch]);
	}
}

// ---------------------------------------------------------------------------
// accumulateNearest
// ---------------------------------------------------------------------------

void LiveStacker::accumulateNearest(preview_image *image, double dx, double dy, double theta) {
	const char *raw = image->m_raw_data;
	const int W = m_width;
	const int H = m_height;
	const int channels = m_channels;
	const int pix_fmt = m_pix_format;
	double *acc = m_acc.data();
	const double cx = (W - 1) * 0.5;
	const double cy = (H - 1) * 0.5;
	const double cos_t = std::cos(theta);
	const double sin_t = std::sin(theta);

	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	std::vector<std::thread> threads;

	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = static_cast<int>(std::ceil(H / (double)num_threads));
		threads.emplace_back([=]() {
			const int start_row = chunk * rank;
			const int end_row = std::min(start_row + chunk, H);
			for (int y = start_row; y < end_row; ++y) {
				const double ry = y - cy;
				for (int x = 0; x < W; ++x) {
					const double rx = x - cx;
					const int sx = static_cast<int>(std::round(rx * cos_t - ry * sin_t + cx + dx));
					const int sy = static_cast<int>(std::round(rx * sin_t + ry * cos_t + cy + dy));

					if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;

					const int dst_idx = y * W + x;
					for (int c = 0; c < channels; ++c) {
						acc[dst_idx * channels + c] += readSample(raw, W, H, pix_fmt, sx, sy, c);
					}
				}
			}
		});
	}
	for (auto &t : threads) t.join();
}

// ---------------------------------------------------------------------------
// accumulateBilinear
// ---------------------------------------------------------------------------

void LiveStacker::accumulateBilinear(preview_image *image, double dx, double dy, double theta) {
	const char *raw = image->m_raw_data;
	const int W = m_width;
	const int H = m_height;
	const int channels = m_channels;
	const int pix_fmt = m_pix_format;
	double *acc = m_acc.data();
	const double cx = (W - 1) * 0.5;
	const double cy = (H - 1) * 0.5;
	const double cos_t = std::cos(theta);
	const double sin_t = std::sin(theta);

	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	std::vector<std::thread> threads;

	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = static_cast<int>(std::ceil(H / (double)num_threads));
		threads.emplace_back([=]() {
			const int start_row = chunk * rank;
			const int end_row = std::min(start_row + chunk, H);
			for (int y = start_row; y < end_row; ++y) {
				const double ry = y - cy;
				for (int x = 0; x < W; ++x) {
					const double rx = x - cx;
					const double sx_d = rx * cos_t - ry * sin_t + cx + dx;
					const double sy_d = rx * sin_t + ry * cos_t + cy + dy;
					const int x0 = static_cast<int>(std::floor(sx_d));
					const int y0 = static_cast<int>(std::floor(sy_d));
					const double fx = sx_d - x0;
					const double fy = sy_d - y0;
					const int x1 = x0 + 1;
					const int y1 = y0 + 1;
					const double w00 = (1.0 - fx) * (1.0 - fy);
					const double w10 = fx         * (1.0 - fy);
					const double w01 = (1.0 - fx) * fy;
					const double w11 = fx         * fy;
					const int dst_idx = y * W + x;
					for (int c = 0; c < channels; ++c) {
						const double val = w00 * readSample(raw, W, H, pix_fmt, x0, y0, c)
						                 + w10 * readSample(raw, W, H, pix_fmt, x1, y0, c)
						                 + w01 * readSample(raw, W, H, pix_fmt, x0, y1, c)
						                 + w11 * readSample(raw, W, H, pix_fmt, x1, y1, c);
						acc[dst_idx * channels + c] += val;
					}
				}
			}
		});
	}
	for (auto &t : threads) t.join();
}

// ---------------------------------------------------------------------------
// accumulateBicubic  (Catmull-Rom, Keys a=-0.5)
// ---------------------------------------------------------------------------

void LiveStacker::accumulateBicubic(preview_image *image, double dx, double dy, double theta) {
	const char *raw = image->m_raw_data;
	const int W = m_width;
	const int H = m_height;
	const int channels = m_channels;
	const int pix_fmt = m_pix_format;
	double *acc = m_acc.data();
	const double cx = (W - 1) * 0.5;
	const double cy = (H - 1) * 0.5;
	const double cos_t = std::cos(theta);
	const double sin_t = std::sin(theta);

	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	std::vector<std::thread> threads;

	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = static_cast<int>(std::ceil(H / (double)num_threads));
		threads.emplace_back([=]() {

			auto cubic = [](double t) -> double {
				t = std::abs(t);
				if (t < 1.0) return (1.5*t - 2.5)*t*t + 1.0;
				if (t < 2.0) return ((-0.5*t + 2.5)*t - 4.0)*t + 2.0;
				return 0.0;
			};

			const int start_row = chunk * rank;
			const int end_row   = std::min(start_row + chunk, H);
			for (int y = start_row; y < end_row; ++y) {
				const double ry = y - cy;
				for (int x = 0; x < W; ++x) {
					const double rx = x - cx;
					const double sx_d = rx * cos_t - ry * sin_t + cx + dx;
					const double sy_d = rx * sin_t + ry * cos_t + cy + dy;
					const int xi = static_cast<int>(std::floor(sx_d));
					const int yi = static_cast<int>(std::floor(sy_d));
					const double fx = sx_d - xi;
					const double fy = sy_d - yi;
					double wx[4], wy[4];
					for (int k = 0; k < 4; ++k) {
						wx[k] = cubic(fx - (k - 1));
						wy[k] = cubic(fy - (k - 1));
					}
					const int dst_idx = y * W + x;
					for (int c = 0; c < channels; ++c) {
						double val = 0.0;
						for (int j = 0; j < 4; ++j) {
							for (int i = 0; i < 4; ++i) {
								val += wx[i] * wy[j] * readSample(raw, W, H, pix_fmt, xi + i - 1, yi + j - 1, c);
							}
						}
						acc[dst_idx * channels + c] += val;
					}
				}
			}
		});
	}
	for (auto &t : threads) t.join();
}

// ---------------------------------------------------------------------------
// accumulate — dispatcher
// ---------------------------------------------------------------------------

void LiveStacker::accumulate(preview_image *image, double dx, double dy, double theta) {
	switch (m_interp_method) {
		case INTERP_NEAREST:
			accumulateNearest(image, dx, dy, theta);
			break;
		case INTERP_BILINEAR:
			accumulateBilinear(image, dx, dy, theta);
			break;
		case INTERP_BICUBIC:
			accumulateBicubic(image, dx, dy, theta);
			break;
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
	// Parallel scan for local maxima with overlap to handle NMS window at chunk boundaries.
	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	std::vector<std::future<std::vector<StarCentroid>>> futures;
	const float *lumData = lum.data();
	const int y_min = nr;
	const int y_max = dH - nr; // exclusive upper bound

	for (int rank = 0; rank < num_threads; rank++) {
		const int chunk = static_cast<int>(std::ceil((double)(y_max - y_min) / (double)num_threads));
		const int start = std::max(y_min, y_min + rank * chunk - nr);
		const int end = std::min(y_max, y_min + (rank + 1) * chunk + nr);
		if (start >= end) continue;
		futures.push_back(std::async(std::launch::async, [=]() -> std::vector<StarCentroid> {
			std::vector<StarCentroid> local;
			const double ds_center_offset = 0.5 * (ds - 1);
			for (int y = start; y < end; ++y) {
				for (int x = nr; x < dW - nr; ++x) {
					float v = lumData[y * dW + x];
					if (v < threshold) continue;
					bool isMax = true;
					for (int dy = -nr; dy <= nr && isMax; ++dy) {
						for (int dx = -nr; dx <= nr && isMax; ++dx) {
							if (dx == 0 && dy == 0) continue;
							if (lumData[(y + dy) * dW + (x + dx)] >= v) isMax = false;
						}
					}
					if (!isMax) continue;

					// Intensity-weighted centroid within the refinement window.
					double cx = 0.0, cy = 0.0, flux = 0.0;
					const int x0 = std::max(0, x - wh);
					const int x1 = std::min(dW - 1, x + wh);
					const int y0 = std::max(0, y - wh);
					const int y1 = std::min(dH - 1, y + wh);
					for (int py = y0; py <= y1; ++py) {
						for (int px = x0; px <= x1; ++px) {
							float w = std::max(0.0f, lumData[py * dW + px]);
							cx += w * px;
							cy += w * py;
							flux += w;
						}
					}
					if (flux < 1e-6f) continue;
					cx /= flux;
					cy /= flux;

					StarCentroid sc;
					// Convert the centroid from downsampled-cell coordinates back to
					// original pixel coordinates using the centre of each ds×ds box.
					// Without this offset the detected stars are all biased by
					// (ds-1)/2 pixels, which is mostly absorbed by translation-only
					// alignment but shifts the effective rotation centre.  At 180°
					// that bias turns into an apparent ~1 px translation for ds=2.
					sc.x = static_cast<float>(cx * ds + ds_center_offset);
					sc.y = static_cast<float>(cy * ds + ds_center_offset);
					sc.flux = static_cast<float>(flux);
					local.push_back(sc);
				}
			}
			return local;
		}));
	}

	// Gather results and keep the brightest stars.
	std::vector<StarCentroid> candidates;
	for (auto &f : futures) {
		std::vector<StarCentroid> part = f.get();
		candidates.insert(candidates.end(), part.begin(), part.end());
	}

	std::sort(candidates.begin(), candidates.end(), [](const StarCentroid &a, const StarCentroid &b){ return a.flux > b.flux; });
	if (static_cast<int>(candidates.size()) > STAR_MAX_COUNT) {
		candidates.resize(STAR_MAX_COUNT);
	}

	return candidates;
}

// ---------------------------------------------------------------------------
// KdTree2D — lightweight 2-D k-d tree over StarCentroid positions.
//
// Build   : O(N log N) — median partitioning, alternating X/Y split axes.
// NN query: O(log N) average — used by findShiftByKdTree.
//
// All nodes live in a flat array; left/right children are integer indices
// (-1 = none).  No heap fragmentation, trivially copyable.
// ---------------------------------------------------------------------------

struct KdNode {
	float x = 0, y = 0, flux = 0;
	int left  = -1;
	int right = -1;
};

class KdTree2D {
public:
	KdTree2D() = default;

	explicit KdTree2D(const std::vector<StarCentroid> &pts) {
		if (pts.empty()) return;
		m_nodes.resize(pts.size());
		for (size_t i = 0; i < pts.size(); ++i) {
			m_nodes[i].x = pts[i].x;
			m_nodes[i].y = pts[i].y;
			m_nodes[i].flux = pts[i].flux;
		}
		std::vector<int> idx(pts.size());
		std::iota(idx.begin(), idx.end(), 0);
		m_root = build(idx, 0, static_cast<int>(idx.size()), 0);
	}

	bool empty() const { return m_nodes.empty(); }

	const KdNode &node(int i) const { return m_nodes[i]; }

	/// Returns the index of the nearest node to (qx, qy).
	/// Sets @p best_d2 to the squared Euclidean distance.  Returns -1 if empty.
	int nearest(float qx, float qy, float &best_d2) const {
		int best = -1;
		best_d2 = std::numeric_limits<float>::max();
		nnSearch(m_root, qx, qy, 0, best, best_d2);
		return best;
	}

private:
	std::vector<KdNode> m_nodes;
	int m_root = -1;

	int build(std::vector<int> &idx, int lo, int hi, int depth) {
		if (lo >= hi) return -1;
		const int mid  = (lo + hi) / 2;
		const int axis = depth & 1;
		std::nth_element(idx.begin() + lo, idx.begin() + mid, idx.begin() + hi,
			[&](int a, int b) {
				return axis == 0 ? m_nodes[a].x < m_nodes[b].x : m_nodes[a].y < m_nodes[b].y;
			});
		const int id = idx[mid];
		m_nodes[id].left = build(idx, lo, mid, depth + 1);
		m_nodes[id].right = build(idx, mid + 1, hi, depth + 1);
		return id;
	}

	void nnSearch(int id, float qx, float qy, int depth, int &best, float &best_d2) const {
		if (id < 0) return;
		const KdNode &n = m_nodes[id];
		const float dx = qx - n.x;
		const float dy = qy - n.y;
		const float d2 = dx*dx + dy*dy;
		if (d2 < best_d2) {
			best_d2 = d2;
			best = id;
		}
		const float split_diff = (depth & 1) == 0 ? dx : dy;

		// Visit the closer half-space first.
		const int first  = split_diff >= 0 ? n.right : n.left;
		const int second = split_diff >= 0 ? n.left  : n.right;
		nnSearch(first,  qx, qy, depth + 1, best, best_d2);

		// Only visit the farther half-space if it can improve the result.
		if (split_diff * split_diff < best_d2) {
			nnSearch(second, qx, qy, depth + 1, best, best_d2);
		}
	}
};

// ---------------------------------------------------------------------------
// findShiftByCentroids
//
// For each star in m_ref_stars, find the nearest star in cur_stars within
// STAR_MATCH_RADIUS original pixels.  The shift is the median (dx, dy) over
// all matched pairs.  Returns false when fewer than STAR_MIN_MATCHES pairs
// are found.
// ---------------------------------------------------------------------------

bool LiveStacker::findShiftByCentroids(double &shift_x, double &shift_y, const std::vector<StarCentroid> &cur_stars) const {
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
				bestDx = ddx;
				bestDy = ddy;
				found = true;
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

	shift_x = dxs[mid];
	shift_y = dys[mid];
	return true;
}

// ---------------------------------------------------------------------------
// tryAlign
//
// Core single-pass rigid-body alignment attempt used by findRotationAndShift.
//
// Step 1 — k-d-tree matching: each reference star is paired with its nearest
//   neighbour in @p stars.
//
// Step 2 — Kabsch 2-D: builds the 2×2 cross-covariance matrix H = Σ ri' ci'^T
//   (centred around the pair centroids, not the image centre, so translation
//   does not bias the angle estimate) and extracts the optimal rotation:
//
//     θ = atan2( H[0,1] − H[1,0] ,  H[0,0] + H[1,1] )
//
//   then computes the median residual translation.
//
// Step 3 — RANSAC cleanup: keeps only inliers within HOUGH_BIN_PX*5 of the
//   initial estimate and reruns Kabsch on them for a cleaner final estimate.
//
// Returns the final inlier count (0 when fewer than STAR_MIN_MATCHES pairs
// could be matched).
// ---------------------------------------------------------------------------

int LiveStacker::tryAlign(const std::vector<StarCentroid> &stars, double &out_theta, double &out_sx, double &out_sy) const {
	out_theta = out_sx = out_sy = 0.0;

	const double cx  = (m_width  - 1) * 0.5;
	const double cy  = (m_height - 1) * 0.5;
	const double tol = static_cast<double>(HOUGH_BIN_PX) * 10.0;

	// --- Step 1: match reference stars to nearest current star ---
	const KdTree2D tree(stars);
	std::vector<AlignPair> pairs;
	pairs.reserve(m_ref_stars.size());
	for (const StarCentroid &ref : m_ref_stars) {
		float best_d2 = std::numeric_limits<float>::max();
		const int idx = tree.nearest(ref.x, ref.y, best_d2);
		if (idx < 0) continue;
		const KdNode &hit = tree.node(idx);
		pairs.push_back({ref.x, ref.y, hit.x, hit.y});
	}
	if (static_cast<int>(pairs.size()) < STAR_MIN_MATCHES) return 0;

	// Kabsch 2-D: given a subset of (ref, cur) pairs compute the optimal
	// rotation angle and median translation, returning the inlier count.
	auto kabsch = [&](const std::vector<AlignPair> &pts, double threshold, double &theta_out, double &sx_out, double &sy_out) -> int {
		if (static_cast<int>(pts.size()) < STAR_MIN_MATCHES) return 0;

		// Centroids.
		double rx_mean = 0, ry_mean = 0, cx_mean = 0, cy_mean = 0;
		for (const AlignPair &p : pts) {
			rx_mean += p.rx;  ry_mean += p.ry;
			cx_mean += p.cx_s; cy_mean += p.cy_s;
		}
		const double inv_N = 1.0 / static_cast<double>(pts.size());
		rx_mean *= inv_N;  ry_mean *= inv_N;
		cx_mean *= inv_N;  cy_mean *= inv_N;

		// Cross-covariance H = Σ ri' ci'^T (centred around pair centroids,
		// NOT image centre, so translation does not bias the angle estimate).
		double H00 = 0, H01 = 0, H10 = 0, H11 = 0;
		for (const AlignPair &p : pts) {
			const double rx_ = p.rx - rx_mean;
			const double ry_ = p.ry - ry_mean;
			const double cx_ = p.cx_s - cx_mean;
			const double cy_ = p.cy_s - cy_mean;
			H00 += rx_ * cx_;
			H01 += rx_ * cy_;
			H10 += ry_ * cx_;
			H11 += ry_ * cy_;
		}

		// Optimal rotation: θ = atan2(H01−H10, H00+H11).
		theta_out = std::atan2(H01 - H10, H00 + H11);

		// Translation: median residual after applying rotation around image centre.
		const double ct = std::cos(theta_out);
		const double st = std::sin(theta_out);
		std::vector<float> dxv, dyv;
		dxv.reserve(pts.size());
		dyv.reserve(pts.size());
		for (const AlignPair &p : pts) {
			const double rot_x = (p.rx - cx) * ct - (p.ry - cy) * st + cx;
			const double rot_y = (p.rx - cx) * st + (p.ry - cy) * ct + cy;
			dxv.push_back(static_cast<float>(p.cx_s - rot_x));
			dyv.push_back(static_cast<float>(p.cy_s - rot_y));
		}
		const size_t m = dxv.size() / 2;
		std::nth_element(dxv.begin(), dxv.begin() + m, dxv.end());
		std::nth_element(dyv.begin(), dyv.begin() + m, dyv.end());
		sx_out = dxv[m];
		sy_out = dyv[m];

		int agree = 0;
		for (const AlignPair &p : pts) {
			const double rot_x = (p.rx - cx) * ct - (p.ry - cy) * st + cx + sx_out;
			const double rot_y = (p.rx - cx) * st + (p.ry - cy) * ct + cy + sy_out;
			if (std::abs(p.cx_s - rot_x) <= threshold && std::abs(p.cy_s - rot_y) <= threshold) {
				++agree;
			}
		}
		return agree;
	};

	// --- Step 2: initial Kabsch estimate on ALL pairs (tolerant threshold) ---
	const double loose_tol = tol * 2.0;
	int score = kabsch(pairs, loose_tol, out_theta, out_sx, out_sy);

	// --- Step 3: one RANSAC-style cleanup pass ---
	// Keep only pairs whose residual is below HOUGH_BIN_PX*5.  Then
	// recompute Kabsch on inliers for a cleaner final estimate.
	const double ct = std::cos(out_theta);
	const double st = std::sin(out_theta);
	const double cleanup_tol = static_cast<double>(HOUGH_BIN_PX) * 5.0;
	std::vector<AlignPair> inliers;
	inliers.reserve(pairs.size());
	for (const AlignPair &p : pairs) {
		const double rot_x = (p.rx - cx) * ct - (p.ry - cy) * st + cx + out_sx;
		const double rot_y = (p.rx - cx) * st + (p.ry - cy) * ct + cy + out_sy;
		if (std::abs(p.cx_s - rot_x) <= cleanup_tol && std::abs(p.cy_s - rot_y) <= cleanup_tol) {
			inliers.push_back(p);
		}
	}
	if (static_cast<int>(inliers.size()) >= STAR_MIN_MATCHES) {
		score = kabsch(inliers, tol, out_theta, out_sx, out_sy);
	}

	return score;
}

// ---------------------------------------------------------------------------
// findRotationAndShift
//
// Estimates a rigid-body transform (rotation θ around image centre +
// residual translation) from matched star pairs by running tryAlign twice:
//   Pass A — current stars as-is (handles 0° … ±~150° rotations).
//   Pass B — current stars pre-rotated 180° around image centre (handles
//             meridian flips and any rotation near ±180°).  The recovered
//             theta is adjusted by +π to give the true angle.
// The pass with more inliers wins.
// ---------------------------------------------------------------------------

bool LiveStacker::findRotationAndShift(double &shift_x, double &shift_y, double &theta, const std::vector<StarCentroid> &cur_stars) const {
	shift_x = shift_y = theta = 0.0;

	if (m_ref_stars.empty() || cur_stars.empty()) return false;

	const double cx = (m_width - 1) * 0.5;
	const double cy = (m_height - 1) * 0.5;

	// -----------------------------------------------------------------------
	// Run alignment twice:
	//   Pass A — current stars as-is (handles 0° … ±~150° rotations).
	//   Pass B — current stars pre-rotated 180° around image centre (handles
	//             meridian flips and any rotation near ±180°).  The recovered
	//             theta is then adjusted by +π to give the true angle in the
	//             original coordinate frame.
	//
	// The pass with more inliers wins.  If both passes have fewer than
	// STAR_MIN_MATCHES inliers the function returns false.
	// -----------------------------------------------------------------------
	double tA, sxA, syA;
	int scoreA = tryAlign(cur_stars, tA, sxA, syA);

	// Build 180°-flipped copy of cur_stars.
	std::vector<StarCentroid> flipped = cur_stars;
	for (StarCentroid &s : flipped) {
		s.x = static_cast<float>(2.0 * cx - s.x);
		s.y = static_cast<float>(2.0 * cy - s.y);
	}
	double tB, sxB, syB;
	int scoreB = tryAlign(flipped, tB, sxB, syB);
	// Adjust theta back to the original frame by adding π.
	tB += M_PI;
	while (tB >  M_PI) tB -= 2.0 * M_PI;
	while (tB < -M_PI) tB += 2.0 * M_PI;
	// The shift was computed against flipped (= 2*c - orig) stars, so the
	// model in the flipped frame is:  flipped_star ≈ R(tB_raw)·ref + sxB
	// i.e.  2*cx - orig_star ≈ R(tB_raw)·ref + sxB
	// =>     orig_star       ≈ R(tB_raw + π)·ref - sxB
	// Therefore the shift in the original coordinate frame is -sxB / -syB.
	sxB = -sxB;
	syB = -syB;

	if (scoreA <= 0 && scoreB <= 0) return false;

	if (scoreA >= scoreB) {
		theta = tA; shift_x = sxA; shift_y = syA;
	} else {
		theta = tB; shift_x = sxB; shift_y = syB;
	}

	if (std::max(scoreA, scoreB) < STAR_MIN_MATCHES) {
		shift_x = shift_y = theta = 0.0;
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// findShiftByHough
//
// Hough-style translation histogram (also called "generalised Hough transform
// for translation" or simply "voting alignment").
//
// For every ordered pair (ref_star_i, cur_star_j) vote for the translation
//
//   T = (cur_j.x − ref_i.x,  cur_j.y − ref_i.y)
//
// with weight = sqrt(ref_i.flux × cur_j.flux) (geometric mean — bright, clean
// stars contribute more than faint noisy ones).
//
// Votes are accumulated into a 2-D histogram with HOUGH_BIN_PX-sized bins
// covering ±SEARCH_PX in both axes.  Because "wrong" pairs (i ≠ j as physical
// stars) produce votes scattered uniformly across all bins, while every
// correctly corresponding pair votes for the same bin, the true translation
// appears as a sharp spike regardless of field density or match-radius.
//
// Sub-bin precision: after finding the peak bin, a flux-weighted centroid
// over the 3×3 neighbourhood refines the result to ≈1 original pixel.
//
// Reliability gate: the number of (ref, cur) pairs whose vote falls in the
// 3×3 neighbourhood of the peak must reach STAR_MIN_MATCHES; otherwise the
// detection is considered unreliable and false is returned.
//
// Complexity: O(N×M) — for N=M=100 stars this is only 10 000 iterations.
// ---------------------------------------------------------------------------

bool LiveStacker::findShiftByHough(double &shift_x, double &shift_y, const std::vector<StarCentroid> &cur_stars) const {
	if (m_ref_stars.empty() || cur_stars.empty()) return false;

	// Histogram geometry
	const int BIN = HOUGH_BIN_PX;
	const int RANGE = SEARCH_PX;             // half-range in original pixels
	const int BINS = 2 * RANGE / BIN + 1;    // e.g. 2*512/4+1 = 257
	const int CENTRE = RANGE / BIN;          // index corresponding to delta=0

	std::vector<float> hist(static_cast<size_t>(BINS) * BINS, 0.0f);

	// --- Voting pass ---
	for (const StarCentroid &ref : m_ref_stars) {
		for (const StarCentroid &cur : cur_stars) {
			const float ddx = cur.x - ref.x;
			const float ddy = cur.y - ref.y;
			if (ddx < -RANGE || ddx > RANGE || ddy < -RANGE || ddy > RANGE) continue;
			const int bx = static_cast<int>(std::round(ddx / BIN)) + CENTRE;
			const int by = static_cast<int>(std::round(ddy / BIN)) + CENTRE;
			if (bx < 0 || bx >= BINS || by < 0 || by >= BINS) continue;
			const float weight = std::sqrt(ref.flux * cur.flux);
			hist[by * BINS + bx] += weight;
		}
	}

	// --- Find peak bin ---
	const auto it = std::max_element(hist.begin(), hist.end());
	if (*it <= 0.0f) {
		shift_x = shift_y = 0;
		return false;
	}

	const int peak_idx = static_cast<int>(it - hist.begin());
	const int peak_bx  = peak_idx % BINS;
	const int peak_by  = peak_idx / BINS;

	// --- Sub-bin refinement: flux-weighted centroid over 3×3 neighbourhood ---
	double cx = 0.0, cy = 0.0, wsum = 0.0;
	for (int dy = -1; dy <= 1; ++dy) {
		for (int dx = -1; dx <= 1; ++dx) {
			const int nx = peak_bx + dx;
			const int ny = peak_by + dy;
			if (nx < 0 || nx >= BINS || ny < 0 || ny >= BINS) continue;
			const float w = hist[ny * BINS + nx];
			cx   += w * nx;
			cy   += w * ny;
			wsum += w;
		}
	}
	if (wsum < 1e-9) {
		shift_x = shift_y = 0;
		return false;
	}
	cx /= wsum;
	cy /= wsum;

	shift_x = (cx - CENTRE) * BIN;
	shift_y = (cy - CENTRE) * BIN;

	// --- Reliability gate: count how many star pairs voted into the peak neighbourhood ---
	int vote_count = 0;
	for (const StarCentroid &ref : m_ref_stars) {
		for (const StarCentroid &cur : cur_stars) {
			const float ddx = cur.x - ref.x;
			const float ddy = cur.y - ref.y;
			const int bx = static_cast<int>(std::round(ddx / BIN)) + CENTRE;
			const int by = static_cast<int>(std::round(ddy / BIN)) + CENTRE;
			if (std::abs(bx - peak_bx) <= 1 && std::abs(by - peak_by) <= 1) {
				++vote_count;
			}
		}
	}
	return (vote_count >= STAR_MIN_MATCHES);
}

// ---------------------------------------------------------------------------
// findShiftByKdTree
//
// Builds a 2-D k-d tree over cur_stars (O(M log M)), then for every reference
// star queries the true nearest neighbour without any distance restriction
// (O(log M) per query → O(N log M) total).  All N shifts are collected and
// the median (dx, dy) is taken as the frame shift — the median is inherently
// robust: even if half the matches are wrong (stars that appear in both frames
// but are not the same physical star), the median still converges to the
// correct translation.
//
// Compared to ALIGN_CENTROIDS:
//   • No STAR_MATCH_RADIUS gate  →  works for large unknown shifts.
//   • Relies solely on median robustness instead of a hard accept/reject.
//   • O(N log M) vs O(N × M)     →  5-10× faster for 100-star lists.
//
// A reliability check after the median counts how many pairs agree with it
// within HOUGH_BIN_PX pixels; the method returns false when
// fewer than STAR_MIN_MATCHES pairs qualify.
// ---------------------------------------------------------------------------

bool LiveStacker::findShiftByKdTree(double &shift_x, double &shift_y, const std::vector<StarCentroid> &cur_stars) const {
	if (m_ref_stars.empty() || cur_stars.empty()) return false;

	// Build the tree over cur_stars once — O(M log M).
	const KdTree2D tree(cur_stars);

	std::vector<float> dxs, dys;
	dxs.reserve(m_ref_stars.size());
	dys.reserve(m_ref_stars.size());

	for (const StarCentroid &ref : m_ref_stars) {
		float best_d2 = std::numeric_limits<float>::max();
		const int idx = tree.nearest(ref.x, ref.y, best_d2);
		if (idx < 0) continue;
		const KdNode &hit = tree.node(idx);
		dxs.push_back(hit.x - ref.x);
		dys.push_back(hit.y - ref.y);
	}

	if (static_cast<int>(dxs.size()) < STAR_MIN_MATCHES) {
		shift_x = shift_y = 0;
		return false;
	}

	// Robust median shift.
	const size_t mid = dxs.size() / 2;
	std::nth_element(dxs.begin(), dxs.begin() + mid, dxs.end());
	std::nth_element(dys.begin(), dys.begin() + mid, dys.end());
	shift_x = dxs[mid];
	shift_y = dys[mid];

	// Reliability gate: count pairs within HOUGH_BIN_PX of the median.
	const float tol = static_cast<float>(HOUGH_BIN_PX);
	int agree = 0;
	for (size_t i = 0; i < dxs.size(); ++i) {
		if (std::abs(dxs[i] - static_cast<float>(shift_x)) <= tol && std::abs(dys[i] - static_cast<float>(shift_y)) <= tol) {
			++agree;
		}
	}
	if (agree < STAR_MIN_MATCHES) {
		shift_x = shift_y = 0;
		return false;
	}
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

	const auto t0 = std::chrono::steady_clock::now();

	if (m_frame_count == 0) {
		m_width = W;
		m_height = H;
		m_channels = ch;
		m_pix_format = fmt;
		m_acc.assign(static_cast<size_t>(W) * H * ch, 0.0);

		m_ref_stars = detectStars(image);

		accumulate(image, 0, 0);
	} else {
		if (W != m_width || H != m_height || fmt != m_pix_format)
			return false;

		double dx = 0.0, dy = 0.0, theta = 0.0;

		bool aligned = false;
		if (!m_ref_stars.empty()) {
			std::vector<StarCentroid> cur_stars = detectStars(image);
			if (m_alignment_method == ALIGN_KD_TREE_ROTATION) {
				aligned = findRotationAndShift(dx, dy, theta, cur_stars);
				if (aligned) {
					indigo_error("LiveStacker::addImage: rotation %.4f deg, shift (%.2f, %.2f)\n", theta * 180.0 / M_PI, dx, dy);
				} else {
					// Rotation estimate failed — fall back to KD_TREE translation-only.
					aligned = findShiftByKdTree(dx, dy, cur_stars);
					if (aligned) {
						indigo_error("LiveStacker::addImage: rotation failed, translation-only shift (%.2f, %.2f)\n", dx, dy);
					}
				}
			} else if (m_alignment_method == ALIGN_HOUGH) {
				aligned = findShiftByHough(dx, dy, cur_stars);
			} else if (m_alignment_method == ALIGN_KD_TREE) {
				aligned = findShiftByKdTree(dx, dy, cur_stars);
			} else {
				aligned = findShiftByCentroids(dx, dy, cur_stars);
			}
		}

		if (!aligned) {
			indigo_error("LiveStacker::addImage: alignment failed, stacking without shift\n");
		}

		accumulate(image, dx, dy, theta);
	}

	++m_frame_count;

	const auto t1 = std::chrono::steady_clock::now();
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	indigo_error("LiveStacker::addImage: frame added in %lld ms using %d cores\n", (long long)ms, num_threads);

	return true;
}

// ---------------------------------------------------------------------------
// currentStack
// ---------------------------------------------------------------------------

preview_image *LiveStacker::currentStack() const {
	if (m_frame_count == 0) return nullptr;

	const int samples = m_width * m_height * m_channels;

	const size_t byte_size = static_cast<size_t>(samples) * sizeof(float);
	auto owner = std::shared_ptr<char>(new char[byte_size], std::default_delete<char[]>());
	float *dst = reinterpret_cast<float *>(owner.get());

	const double inv = 1.0 / m_frame_count;

	int num_threads = get_number_of_cores();
	num_threads = (num_threads > 0) ? num_threads : AIN_DEFAULT_THREADS;
	std::vector<std::thread> threads;
	const double *src = m_acc.data();
	for (int rank = 0; rank < num_threads; rank++) {
		const size_t chunk = static_cast<size_t>(std::ceil(samples / (double)num_threads));
		threads.emplace_back([=]() {
			const size_t start = chunk * rank;
			const size_t end = std::min(start + chunk, static_cast<size_t>(samples));
			for (size_t i = start; i < end; ++i) {
				dst[i] = static_cast<float>(src[i] * inv);
			}
		});
	}
	for (auto &t : threads) t.join();

	const int out_fmt = (m_channels == 1) ? PIX_FMT_F32 : PIX_FMT_RGBF;
	stretch_config_t sconfig{};
	return create_preview(m_width, m_height, out_fmt, owner, owner.get(), sconfig);
}
