#pragma once

#include <vector>
#include <QPointF>

// forward
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
};

class ImageInspector {
public:
    ImageInspector();
    ~ImageInspector();

    // Inspect the provided preview image and return results.
    // Parameters like grid size and thresholds may be tuned later.
    InspectionResult inspect(const preview_image &img, int gx = 5, int gy = 5, double inspection_snr = 8.0);
};
