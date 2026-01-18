#include "image_inspector.h"
#include "imagepreview.h"
#include "snr_calculator.h"
#include <cmath>
#include <algorithm>
#include <future>
#include <cstdlib>

// =============================================================================
// ImagePreprocessor Implementation
// =============================================================================

bool ImagePreprocessor::isColorFormat(int pix_format) {
	return pix_format == PIX_FMT_RGB24 ||
	       pix_format == PIX_FMT_RGB48 ||
	       pix_format == PIX_FMT_RGB96 ||
	       pix_format == PIX_FMT_RGBF;
}

std::unique_ptr<preview_image> ImagePreprocessor::convertToFloat32Grayscale(const preview_image& img) {
	int width = img.width();
	int height = img.height();

	std::unique_ptr<preview_image> gray(new preview_image(width, height, QImage::Format_Grayscale8));
	gray->m_width = width;
	gray->m_height = height;
	gray->m_pix_format = PIX_FMT_F32;

	size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(float);
	char* tmpbuf = static_cast<char*>(malloc(size));
	if (!tmpbuf) {
		return nullptr;
	}

	gray->m_raw_owner = std::shared_ptr<char>(tmpbuf, [](char* p) { free(p); });
	gray->m_raw_data = gray->m_raw_owner.get();

	// Copy WCS/meta if present
	gray->m_center_ra = img.m_center_ra;
	gray->m_center_dec = img.m_center_dec;
	gray->m_telescope_ra = img.m_telescope_ra;
	gray->m_telescope_dec = img.m_telescope_dec;
	gray->m_rotation_angle = img.m_rotation_angle;
	gray->m_parity = img.m_parity;
	gray->m_pix_scale = img.m_pix_scale;

	float* dst = reinterpret_cast<float*>(gray->m_raw_data);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double r = 0, g = 0, b = 0;
			img.pixel_value(x, y, r, g, b);
			dst[y * width + x] = static_cast<float>(r + g + b);
		}
	}

	return gray;
}

const preview_image* ImagePreprocessor::toGrayscale(
	const preview_image& img,
	std::unique_ptr<preview_image>& converted_out,
	std::string& error_out
) {
	double tr = 0, tg = 0, tb = 0;
	int pf = img.pixel_value(0, 0, tr, tg, tb);

	if (!isColorFormat(pf)) {
		// Already grayscale, return input directly
		return &img;
	}

	converted_out = convertToFloat32Grayscale(img);
	if (!converted_out) {
		error_out = "Failed to allocate memory for grayscale preview";
		return nullptr;
	}

	return converted_out.get();
}

// =============================================================================
// BackgroundEstimator Implementation
// =============================================================================

BackgroundEstimator::Result BackgroundEstimator::computeMedianMAD(
	const preview_image& img,
	int x0, int y0, int x1, int y1
) {
	Result result;

	std::vector<double> vals;
	vals.reserve((x1 - x0 + 1) * (y1 - y0 + 1));

	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {
			double v = 0, g = 0, b = 0;
			img.pixel_value(x, y, v, g, b);
			vals.push_back(v);
		}
	}

	if (vals.empty()) {
		return result;
	}

	std::sort(vals.begin(), vals.end());
	size_t n = vals.size();

	// Compute median
	result.median = (n % 2 == 1)
		? vals[n / 2]
		: 0.5 * (vals[n / 2 - 1] + vals[n / 2]);

	// Compute MAD (Median Absolute Deviation)
	std::vector<double> devs;
	devs.reserve(n);
	for (double v : vals) {
		devs.push_back(std::fabs(v - result.median));
	}
	std::sort(devs.begin(), devs.end());

	double mad = (n % 2 == 1)
		? devs[n / 2]
		: 0.5 * (devs[n / 2 - 1] + devs[n / 2]);

	// Convert MAD to approximate standard deviation (for normal distribution)
	result.sigma = mad * 1.4826;

	return result;
}

// =============================================================================
// StarDetector Implementation
// =============================================================================

StarDetector::StarDetector(const InspectorConfig& config)
	: m_config(config)
{}

bool StarDetector::isLocalMaximum(
	const preview_image& img,
	int x, int y,
	double threshold
) const {
	double center_val = 0, gv = 0, bv = 0;
	img.pixel_value(x, y, center_val, gv, bv);

	if (center_val <= threshold) {
		return false;
	}

	int img_w = img.width();
	int img_h = img.height();

	for (int ny = y - 1; ny <= y + 1; ++ny) {
		for (int nx = x - 1; nx <= x + 1; ++nx) {
			if (nx == x && ny == y) continue;
			if (nx < 0 || nx >= img_w || ny < 0 || ny >= img_h) continue;

			double nval = 0, ng = 0, nb = 0;
			img.pixel_value(nx, ny, nval, ng, nb);

			if (nval > center_val) {
				return false;
			}
		}
	}

	return true;
}

bool StarDetector::measureStar(
	const preview_image& img,
	int peak_x, int peak_y,
	StarCandidate& out
) const {
	SNRResult r = calculateSNR(
		reinterpret_cast<const uint8_t*>(img.m_raw_data),
		img.width(), img.height(),
		img.m_pix_format,
		peak_x, peak_y
	);

	if (!r.valid || r.is_saturated) {
		return false;
	}

	out.x = r.star_x;
	out.y = r.star_y;
	out.hfd = r.hfd;
	out.star_radius = r.star_radius;
	out.snr = r.snr;
	out.moment_m20 = r.moment_m20;
	out.moment_m02 = r.moment_m02;
	out.moment_m11 = r.moment_m11;
	out.eccentricity = r.eccentricity;

	return true;
}

std::vector<StarCandidate> StarDetector::detectInCell(
	const preview_image& img,
	int cell_x, int cell_y,
	int cell_w, int cell_h,
	int grid_x, int grid_y
) const {
	std::vector<StarCandidate> candidates;

	int img_w = img.width();
	int img_h = img.height();
	int cell_index = cell_y * grid_x + cell_x;

	// Cell boundaries
	int x0 = cell_x * cell_w;
	int y0 = cell_y * cell_h;
	int x1 = (cell_x == grid_x - 1) ? img_w - 1 : x0 + cell_w - 1;
	int y1 = (cell_y == grid_y - 1) ? img_h - 1 : y0 + cell_h - 1;

	// Expanded search area
	int sx0 = std::max(0, x0 - m_config.search_margin);
	int sy0 = std::max(0, y0 - m_config.search_margin);
	int sx1 = std::min(img_w - 1, x1 + m_config.search_margin);
	int sy1 = std::min(img_h - 1, y1 + m_config.search_margin);

	// Compute robust background in search area
	auto bg = BackgroundEstimator::computeMedianMAD(img, sx0, sy0, sx1, sy1);
	double peak_threshold = bg.median + m_config.detection_threshold_sigma * bg.sigma;

	// Scan for local maxima
	for (int y = sy0; y <= sy1; ++y) {
		for (int x = sx0; x <= sx1; ++x) {
			if (!isLocalMaximum(img, x, y, peak_threshold)) {
				continue;
			}

			StarCandidate candidate;
			if (!measureStar(img, x, y, candidate)) {
				continue;
			}

			// Check if centroid is within allowed margin of cell
			int cx_i = static_cast<int>(std::round(candidate.x));
			int cy_i = static_cast<int>(std::round(candidate.y));

			if (cx_i >= x0 - m_config.centroid_margin &&
			    cx_i <= x1 + m_config.centroid_margin &&
			    cy_i >= y0 - m_config.centroid_margin &&
			    cy_i <= y1 + m_config.centroid_margin) {
				candidate.cell_index = cell_index;
				candidates.push_back(candidate);
			}
		}
	}

	return candidates;
}

// =============================================================================
// CandidateFilter Implementation
// =============================================================================

CandidateFilter::CandidateFilter(const InspectorConfig& config)
	: m_config(config)
{}

std::vector<StarCandidate> CandidateFilter::deduplicateWithinCell(
	const std::vector<StarCandidate>& candidates
) const {
	std::vector<StarCandidate> unique;

	for (const auto& c : candidates) {
		if (!(c.snr > m_config.snr_threshold && c.hfd > 0)) {
			continue;
		}

		bool replaced = false;
		for (auto& u : unique) {
			double dx = c.x - u.x;
			double dy = c.y - u.y;
			double dist = std::sqrt(dx * dx + dy * dy);

			if (dist <= m_config.duplicate_radius) {
				if (c.snr > u.snr) {
					u = c;
				}
				replaced = true;
				break;
			}
		}

		if (!replaced) {
			unique.push_back(c);
		}
	}

	return unique;
}

std::vector<StarCandidate> CandidateFilter::deduplicateGlobal(
	const std::vector<StarCandidate>& candidates
) const {
	std::vector<StarCandidate> unique;

	for (const auto& c : candidates) {
		bool merged = false;

		for (auto& u : unique) {
			double dx = u.x - c.x;
			double dy = u.y - c.y;
			double dist = std::sqrt(dx * dx + dy * dy);
			double merge_threshold = u.star_radius + c.star_radius;

			if (merge_threshold > 0.0 && dist <= merge_threshold) {
				if (c.snr > u.snr) {
					// Keep the higher SNR candidate position/properties
					// but keep ORIGINAL cell ownership (from first-seen)
					int first_cell = u.cell_index;
					u.x = c.x;
					u.y = c.y;
					u.snr = c.snr;
					u.star_radius = c.star_radius;
					u.hfd = c.hfd;
					u.eccentricity = c.eccentricity;
					u.moment_m20 = c.moment_m20;
					u.moment_m02 = c.moment_m02;
					u.moment_m11 = c.moment_m11;
					u.cell_index = first_cell;
				}
				merged = true;
				break;
			}
		}

		if (!merged) {
			unique.push_back(c);
		}
	}

	return unique;
}

std::vector<size_t> CandidateFilter::sigmaClipHFD(
	const std::vector<StarCandidate>& candidates
) const {
	std::vector<size_t> used_indices;

	if (candidates.empty()) {
		return used_indices;
	}

	// Build (hfd, index) pairs
	std::vector<std::pair<double, size_t>> hv;
	for (size_t i = 0; i < candidates.size(); ++i) {
		hv.emplace_back(candidates[i].hfd, i);
	}

	// Iterative sigma clipping
	auto v = hv;
	for (int iter = 0; iter < 2; ++iter) {
		if (v.empty()) break;

		double sum = 0.0, sumsq = 0.0;
		for (const auto& p : v) {
			sum += p.first;
			sumsq += p.first * p.first;
		}

		double mean = sum / v.size();
		double var = sumsq / v.size() - mean * mean;
		double sd = var > 0 ? std::sqrt(var) : 0.0;

		std::vector<std::pair<double, size_t>> nv;
		for (const auto& p : v) {
			if (std::fabs(p.first - mean) <= m_config.hfd_outlier_threshold * sd) {
				nv.push_back(p);
			}
		}

		if (nv.size() == v.size() || nv.empty()) {
			break;
		}
		v.swap(nv);
	}

	for (const auto& p : v) {
		used_indices.push_back(p.second);
	}

	return used_indices;
}

// =============================================================================
// CellAnalyzer Implementation
// =============================================================================

CellAnalyzer::CellAnalyzer(const InspectorConfig& config)
	: m_config(config)
	, m_detector(config)
	, m_filter(config)
{}

void CellAnalyzer::computeMorphology(
	const std::vector<StarCandidate>& used,
	double& eccentricity_out,
	double& angle_out
) const {
	double m20 = 0.0, m02 = 0.0, m11 = 0.0;
	double ecc_sum = 0.0, total_w = 0.0;

	for (const auto& c : used) {
		double w = std::max(1.0, c.snr);
		m20 += w * c.moment_m20;
		m02 += w * c.moment_m02;
		m11 += w * c.moment_m11;
		ecc_sum += w * c.eccentricity;
		total_w += w;
	}

	if (total_w > 0.0) {
		m20 /= total_w;
		m02 /= total_w;
		m11 /= total_w;

		// Compute major axis angle
		double ang_rad = 0.5 * std::atan2(2.0 * m11, m02 - m20);
		double ang_deg = ang_rad * 180.0 / M_PI;
		while (ang_deg < 0) ang_deg += 180.0;
		while (ang_deg >= 180.0) ang_deg -= 180.0;

		eccentricity_out = ecc_sum / total_w;
		angle_out = ang_deg;
	} else {
		eccentricity_out = 0.0;
		angle_out = 0.0;
	}
}

CellStatistics CellAnalyzer::analyze(
	const preview_image& img,
	int cell_x, int cell_y,
	int cell_w, int cell_h,
	int grid_x, int grid_y
) const {
	CellStatistics stats;
	stats.cell_index = cell_y * grid_x + cell_x;

	// Step 1: Detect all candidates in cell
	auto candidates = m_detector.detectInCell(
		img, cell_x, cell_y, cell_w, cell_h, grid_x, grid_y
	);

	// Step 2: Deduplicate within cell
	auto unique = m_filter.deduplicateWithinCell(candidates);

	// Step 3: Sigma-clip HFD values
	auto used_indices = m_filter.sigmaClipHFD(unique);

	// Step 4: Compute cell HFD (mean of kept values)
	if (!used_indices.empty()) {
		double sum = 0.0;
		for (size_t idx : used_indices) {
			sum += unique[idx].hfd;
		}
		stats.hfd = sum / used_indices.size();
	}

	// Step 5: Separate used vs rejected candidates
	std::vector<bool> is_used(unique.size(), false);
	for (size_t idx : used_indices) {
		is_used[idx] = true;
	}

	for (size_t i = 0; i < unique.size(); ++i) {
		if (is_used[i]) {
			stats.used_candidates.push_back(unique[i]);
		} else {
			stats.rejected_candidates.push_back(unique[i]);
		}
	}

	stats.detected = static_cast<int>(unique.size());
	stats.used = static_cast<int>(stats.used_candidates.size());
	stats.rejected = static_cast<int>(stats.rejected_candidates.size());

	// Step 6: Compute morphology
	computeMorphology(stats.used_candidates, stats.eccentricity, stats.major_angle_deg);

	return stats;
}

// =============================================================================
// GridAnalyzer Implementation
// =============================================================================

GridAnalyzer::GridAnalyzer(const InspectorConfig& config)
	: m_config(config)
	, m_cell_analyzer(config)
{}

std::vector<std::pair<int, int>> GridAnalyzer::getTargetCells() const {
	int gx = m_config.grid_x;
	int gy = m_config.grid_y;

	std::vector<std::pair<int, int>> targets;

	// 4 corners
	targets.emplace_back(0, 0);
	targets.emplace_back(gx - 1, 0);
	targets.emplace_back(gx - 1, gy - 1);
	targets.emplace_back(0, gy - 1);

	// 4 mid-edges
	targets.emplace_back(gx / 2, 0);
	targets.emplace_back(gx - 1, gy / 2);
	targets.emplace_back(gx / 2, gy - 1);
	targets.emplace_back(0, gy / 2);

	// Center
	targets.emplace_back(gx / 2, gy / 2);

	return targets;
}

std::vector<CellStatistics> GridAnalyzer::analyzeTargetCells(
	const preview_image& img
) const {
	int gx = m_config.grid_x;
	int gy = m_config.grid_y;
	int cell_w = img.width() / gx;
	int cell_h = img.height() / gy;

	auto targets = getTargetCells();

	// Launch parallel analysis
	std::vector<std::future<CellStatistics>> futures;
	futures.reserve(targets.size());

	for (size_t i = 0; i < targets.size(); ++i) {
		int cx = targets[i].first;
		int cy = targets[i].second;
		futures.push_back(std::async(
			std::launch::async,
			[this, &img, cx, cy, cell_w, cell_h, gx, gy]() {
				return m_cell_analyzer.analyze(img, cx, cy, cell_w, cell_h, gx, gy);
			}
		));
	}

	// Collect results
	std::vector<CellStatistics> results;
	results.reserve(futures.size());

	for (auto& f : futures) {
		results.push_back(f.get());
	}

	return results;
}

// =============================================================================
// ResultAssembler Implementation
// =============================================================================

ResultAssembler::ResultAssembler(const InspectorConfig& config)
	: m_config(config)
{}

int ResultAssembler::cellToDirectionIndex(int cell_x, int cell_y) const {
	int gx = m_config.grid_x;
	int gy = m_config.grid_y;
	int cx = gx / 2;
	int cy = gy / 2;

	// Map cell position to direction index (0-7, clockwise from top)
	if (cell_x == cx && cell_y == 0) return 0;         // N
	if (cell_x == gx - 1 && cell_y == 0) return 1;     // NE
	if (cell_x == gx - 1 && cell_y == cy) return 2;    // E
	if (cell_x == gx - 1 && cell_y == gy - 1) return 3; // SE
	if (cell_x == cx && cell_y == gy - 1) return 4;    // S
	if (cell_x == 0 && cell_y == gy - 1) return 5;     // SW
	if (cell_x == 0 && cell_y == cy) return 6;         // W
	if (cell_x == 0 && cell_y == 0) return 7;          // NW

	return -1; // Not a direction cell
}

bool ResultAssembler::isCenterCell(int cell_x, int cell_y) const {
	return cell_x == m_config.grid_x / 2 && cell_y == m_config.grid_y / 2;
}

InspectionResult ResultAssembler::assemble(
	const std::vector<CellStatistics>& cell_stats,
	const std::vector<StarCandidate>& global_used
) const {
	InspectionResult res;
	int gx = m_config.grid_x;
	int gy = m_config.grid_y;

	res.dirs.resize(8, 0.0);
	res.detected_dirs.resize(8, 0);
	res.used_dirs.resize(8, 0);
	res.rejected_dirs.resize(8, 0);
	res.cell_eccentricity.resize(gx * gy, 0.0);
	res.cell_major_angle.resize(gx * gy, 0.0);

	// Fill per-cell morphology and HFD
	for (const auto& s : cell_stats) {
		if (s.cell_index >= 0 && s.cell_index < gx * gy) {
			res.cell_eccentricity[s.cell_index] = s.eccentricity;
			res.cell_major_angle[s.cell_index] = s.major_angle_deg;
		}
	}

	// Build used points and recompute cell_used counts from global_used
	std::vector<int> cell_used(gx * gy, 0);
	for (const auto& c : global_used) {
		res.used_points.emplace_back(c.x, c.y);
		res.used_radii.push_back(c.star_radius);
		if (c.cell_index >= 0 && c.cell_index < gx * gy) {
			cell_used[c.cell_index]++;
		}
	}

	// Collect all unique candidates from all cells for detected count and rejected points
	std::vector<StarCandidate> all_unique_candidates;
	for (const auto& s : cell_stats) {
		for (const auto& c : s.used_candidates) {
			all_unique_candidates.push_back(c);
		}
		for (const auto& c : s.rejected_candidates) {
			all_unique_candidates.push_back(c);
		}
	}

	// Recompute detected per cell from all_unique_candidates
	std::vector<int> cell_detected(gx * gy, 0);
	for (const auto& u : all_unique_candidates) {
		if (u.cell_index >= 0 && u.cell_index < gx * gy) {
			cell_detected[u.cell_index]++;
		}
	}

	// Compute rejected = detected - used
	std::vector<int> cell_rejected(gx * gy, 0);
	for (int i = 0; i < gx * gy; ++i) {
		cell_rejected[i] = std::max(0, cell_detected[i] - cell_used[i]);
	}

	// Map cell statistics to directions
	for (const auto& s : cell_stats) {
		int cx = s.cell_index % gx;
		int cy = s.cell_index / gx;

		if (isCenterCell(cx, cy)) {
			res.center_hfd = s.hfd;
			res.center_detected = cell_detected[s.cell_index];
			res.center_used = cell_used[s.cell_index];
			res.center_rejected = cell_rejected[s.cell_index];
		} else {
			int dir = cellToDirectionIndex(cx, cy);
			if (dir >= 0 && dir < 8) {
				res.dirs[dir] = s.hfd;
				res.detected_dirs[dir] = cell_detected[s.cell_index];
				res.used_dirs[dir] = cell_used[s.cell_index];
				res.rejected_dirs[dir] = cell_rejected[s.cell_index];
			}
		}
	}

	// Build rejected points: check ALL unique candidates against global_used
	for (const auto& u : all_unique_candidates) {
		bool matched = false;
		for (const auto& g : global_used) {
			double dx = g.x - u.x;
			double dy = g.y - u.y;
			double dist = std::sqrt(dx * dx + dy * dy);
			double threshold = g.star_radius + u.star_radius;
			if ((threshold > 0.0 && dist <= threshold) || dist <= 1.0) {
				matched = true;
				break;
			}
		}
		if (!matched) {
			res.rejected_points.emplace_back(u.x, u.y);
		}
	}

	return res;
}

// =============================================================================
// ImageInspector Implementation
// =============================================================================

ImageInspector::ImageInspector() = default;

ImageInspector::ImageInspector(const InspectorConfig& config)
	: m_config(config)
{}

ImageInspector::~ImageInspector() = default;

bool ImageInspector::validateInput(const preview_image& img, std::string& error_out) const {
	if (img.m_raw_data == nullptr) {
		error_out = "Image has no raw data";
		return false;
	}

	if (img.width() <= 0 || img.height() <= 0) {
		error_out = "Invalid image dimensions";
		return false;
	}

	return true;
}

InspectionResult ImageInspector::inspect(const preview_image& img) const {
	InspectionResult res;

	// Step 1: Validate input
	if (!validateInput(img, res.error_message)) {
		return res;
	}

	// Step 2: Convert to grayscale if needed
	std::unique_ptr<preview_image> converted;
	const preview_image* work_img = ImagePreprocessor::toGrayscale(img, converted, res.error_message);
	if (!work_img) {
		return res;
	}

	// Step 3: Analyze target cells in parallel
	GridAnalyzer grid_analyzer(m_config);
	auto cell_stats = grid_analyzer.analyzeTargetCells(*work_img);

	// Step 4: Collect all used candidates and perform global deduplication
	std::vector<StarCandidate> all_used;
	for (const auto& s : cell_stats) {
		for (const auto& c : s.used_candidates) {
			all_used.push_back(c);
		}
	}

	CandidateFilter filter(m_config);
	auto global_used = filter.deduplicateGlobal(all_used);

	// Step 5: Check minimum star count per cell (after global dedup)
	// Recompute used counts per cell from global_used
	std::vector<int> cell_used_counts(m_config.grid_x * m_config.grid_y, 0);
	for (const auto& c : global_used) {
		if (c.cell_index >= 0 && c.cell_index < m_config.grid_x * m_config.grid_y) {
			cell_used_counts[c.cell_index]++;
		}
	}

	// Check each target cell has at least 3 stars
	GridAnalyzer grid_analyzer_check(m_config);
	auto targets = grid_analyzer_check.getTargetCells();
	for (const auto& target : targets) {
		int cell_idx = target.second * m_config.grid_x + target.first;
		if (cell_used_counts[cell_idx] < 3) {
			res.error_message = "Not enough usable stars detected";
			return res;
		}
	}

	// Step 6: Assemble final result
	ResultAssembler assembler(m_config);
	res = assembler.assemble(cell_stats, global_used);

	return res;
}

InspectionResult ImageInspector::inspect(
	const preview_image& img,
	int gx, int gy,
	double snr_threshold
) const {
	// Create temporary config with specified parameters
	InspectorConfig config = m_config;
	config.grid_x = gx;
	config.grid_y = gy;
	config.snr_threshold = snr_threshold;

	ImageInspector temp_inspector(config);
	return temp_inspector.inspect(img);
}
