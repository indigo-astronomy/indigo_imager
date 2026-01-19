#ifndef IMAGE_INSPECTOR_H
#define IMAGE_INSPECTOR_H

#include <vector>
#include <QPointF>
#include <string>
#include <memory>

class preview_image;

// =============================================================================
// Data Structures
// =============================================================================

// A single star candidate detected in the image
struct StarCandidate {
	double x;
	double y;
	double hfd;
	double star_radius;
	double snr;
	double moment_m20;
	double moment_m02;
	double moment_m11;
	double eccentricity;
	int cell_index;  // which cell owns this candidate
};

// Statistics for a single grid cell
struct CellStatistics {
	int cell_index = -1;
	double hfd = 0.0;
	double eccentricity = 0.0;
	double major_angle_deg = 0.0;
	int detected = 0;
	int used = 0;
	int rejected = 0;
	std::vector<StarCandidate> used_candidates;
	std::vector<StarCandidate> rejected_candidates;
};

// Final inspection result
struct InspectionResult {
	std::vector<double> dirs;              // 8 directions HFD values
	double center_hfd = 0.0;
	std::vector<int> detected_dirs;        // size 8
	std::vector<int> used_dirs;            // size 8
	std::vector<int> rejected_dirs;        // size 8
	int center_detected = 0;
	int center_used = 0;
	int center_rejected = 0;

	std::vector<QPointF> used_points;      // scene coords (image pixels)
	std::vector<double> used_radii;        // radii in image pixels
	std::vector<QPointF> rejected_points;  // scene coords

	std::vector<double> cell_eccentricity; // size gx*gy
	std::vector<double> cell_major_angle;  // degrees, range [0,180)

	std::string error_message;
};

// =============================================================================
// Configuration
// =============================================================================

struct InspectorConfig {
	int grid_x = 5;
	int grid_y = 5;
	double snr_threshold = 10.0;
	double detection_threshold_sigma = 4.0;  // MAD-derived sigma multiplier
	double hfd_outlier_threshold = 1.5;      // sigma-clip threshold
	double duplicate_radius = 5.0;           // for within-cell deduplication
	int search_margin = 8;
	int centroid_margin = 8;
};

// =============================================================================
// Image Preprocessor - handles format conversion
// =============================================================================

class ImagePreprocessor {
public:
	// Convert color image to grayscale if needed
	// Returns pointer to grayscale image (may return input if already grayscale)
	// If conversion was performed, stores converted image in 'converted_out'
	static const preview_image* toGrayscale(
		const preview_image& img,
		std::unique_ptr<preview_image>& converted_out,
		std::string& error_out
	);

private:
	static bool isColorFormat(int pix_format);
	static std::unique_ptr<preview_image> convertToFloat32Grayscale(const preview_image& img);
};

// =============================================================================
// Background Estimator - robust background statistics
// =============================================================================

class BackgroundEstimator {
public:
	struct Result {
		double median = 0.0;
		double sigma = 0.0;  // MAD-derived standard deviation
	};

	// Compute median and MAD-based sigma for a rectangular region
	static Result computeMedianMAD(
		const preview_image& img,
		int x0, int y0, int x1, int y1
	);
};

// =============================================================================
// Star Detector - finds and measures star candidates in a region
// =============================================================================

class StarDetector {
public:
	StarDetector(const InspectorConfig& config);

	// Find all star candidates in a cell region
	std::vector<StarCandidate> detectInCell(
		const preview_image& img,
		int cell_x, int cell_y,
		int cell_w, int cell_h,
		int grid_x, int grid_y
	) const;

private:
	const InspectorConfig& m_config;

	// Check if pixel is a local maximum above threshold
	bool isLocalMaximum(const preview_image& img, int x, int y, double threshold) const;

	// Measure star properties at a peak position
	bool measureStar(
		const preview_image& img,
		int peak_x, int peak_y,
		StarCandidate& out
	) const;
};

// =============================================================================
// Candidate Filter - deduplication and outlier rejection
// =============================================================================

class CandidateFilter {
public:
	CandidateFilter(const InspectorConfig& config);

	// Remove duplicates within a cell, keeping highest-SNR
	std::vector<StarCandidate> deduplicateWithinCell(
		const std::vector<StarCandidate>& candidates
	) const;

	// Global deduplication across all cells
	std::vector<StarCandidate> deduplicateGlobal(
		const std::vector<StarCandidate>& candidates
	) const;

	// Apply sigma-clipping to HFD values, returns indices of kept candidates
	std::vector<size_t> sigmaClipHFD(
		const std::vector<StarCandidate>& candidates
	) const;

private:
	const InspectorConfig& m_config;
};

// =============================================================================
// Cell Analyzer - processes a single grid cell
// =============================================================================

class CellAnalyzer {
public:
	CellAnalyzer(const InspectorConfig& config);

	// Analyze a single cell and return statistics
	CellStatistics analyze(
		const preview_image& img,
		int cell_x, int cell_y,
		int cell_w, int cell_h,
		int grid_x, int grid_y
	) const;

private:
	const InspectorConfig& m_config;
	StarDetector m_detector;
	CandidateFilter m_filter;

	// Compute weighted eccentricity and angle from candidates
	void computeMorphology(
		const std::vector<StarCandidate>& used,
		double& eccentricity_out,
		double& angle_out
	) const;
};

// =============================================================================
// Grid Analyzer - coordinates parallel cell analysis
// =============================================================================

class GridAnalyzer {
public:
	GridAnalyzer(const InspectorConfig& config);

	// Analyze all target cells (corners, edges, center) in parallel
	std::vector<CellStatistics> analyzeTargetCells(
		const preview_image& img
	) const;

	// Get the list of target cell indices (9 cells: 4 corners + 4 edges + center)
	std::vector<std::pair<int, int>> getTargetCells() const;

private:
	const InspectorConfig& m_config;
	CellAnalyzer m_cell_analyzer;
};

// =============================================================================
// Result Assembler - builds final InspectionResult from cell statistics
// =============================================================================

class ResultAssembler {
public:
	ResultAssembler(const InspectorConfig& config);

	// Assemble final result from cell statistics
	InspectionResult assemble(
		const std::vector<CellStatistics>& cell_stats,
		const std::vector<StarCandidate>& all_candidates
	) const;

private:
	const InspectorConfig& m_config;

	// Map cell coordinates to 8-direction index
	int cellToDirectionIndex(int cell_x, int cell_y) const;

	// Check if cell is the center cell
	bool isCenterCell(int cell_x, int cell_y) const;
};

// =============================================================================
// Main ImageInspector class - orchestrates the pipeline
// =============================================================================

class ImageInspector {
public:
	ImageInspector();
	explicit ImageInspector(const InspectorConfig& config);
	~ImageInspector();

	// Main inspection method
	InspectionResult inspect(const preview_image& img) const;

	// Overload with custom grid size and SNR threshold (for backward compatibility)
	InspectionResult inspect(
		const preview_image& img,
		int gx, int gy,
		double snr_threshold
	) const;

	// Access configuration
	const InspectorConfig& config() const { return m_config; }
	void setConfig(const InspectorConfig& config) { m_config = config; }

private:
	InspectorConfig m_config;

	// Validate input image
	bool validateInput(const preview_image& img, std::string& error_out) const;
};

#endif // IMAGE_INSPECTOR_H
