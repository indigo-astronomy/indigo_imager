#ifndef IMAGE_INSPECTOR_H
#define IMAGE_INSPECTOR_H

#include <vector>
#include <QPointF>
#include <string>

class preview_image;

struct InspectionResult {
	std::vector<double> dirs; // 8 dirs
	double center_hfd = 0.0;
	std::vector<int> detected_dirs; // size 8
	std::vector<int> used_dirs; // size 8
	std::vector<int> rejected_dirs; // size 8
	int center_detected = 0;
	int center_used = 0;
	int center_rejected = 0;

	std::vector<QPointF> used_points; // scene coords (image pixels)
	std::vector<double> used_radii; // radii in image pixels
	std::vector<QPointF> rejected_points; // scene coords
	// per-cell morphology: eccentricity and major-axis angle (degrees)
	std::vector<double> cell_eccentricity; // size gx*gy (filled by inspector)
	std::vector<double> cell_major_angle;  // degrees, range [0,180)

	// Error message (empty when no error). If non-empty the overlay will display
	// this message and skip other visualizations.
	std::string error_message;
};

class ImageInspector {
public:
	ImageInspector();
	~ImageInspector();

	// Inspect the provided preview image and return results.
	// Parameters like grid size and thresholds may be tuned later.
	InspectionResult inspect(const preview_image &img, int gx = 5, int gy = 5, double snr_threshold = 10.0);
};

#endif // IMAGE_INSPECTOR_H