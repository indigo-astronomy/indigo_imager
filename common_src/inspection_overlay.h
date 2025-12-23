#ifndef INSPECTION_OVERLAY_H
#define INSPECTION_OVERLAY_H

#include <QWidget>
#include <vector>
#include "snr_calculator.h"
#include <QFutureWatcher>
#include <atomic>
#include "image_inspector.h"

class QGraphicsView;
class preview_image;
class ImageInspector;

class InspectionOverlay : public QWidget {
	Q_OBJECT

public:
	explicit InspectionOverlay(QWidget *parent = nullptr);
	// Backward-compatible: simple result (no counts)
	void setInspectionResult(const std::vector<double> &directions, double center_hfd);

	// New: result with per-direction counts
	void setInspectionResult(const std::vector<double> &directions, double center_hfd,
		const std::vector<int> &detected, const std::vector<int> &used, const std::vector<int> &rejected,
		int center_detected, int center_used, int center_rejected);

	// New: include per-star positions (in view coordinates) for used/rejected markers
	void setInspectionResult(const std::vector<double> &directions, double center_hfd,
		const std::vector<int> &detected, const std::vector<int> &used, const std::vector<int> &rejected,
		int center_detected, int center_used, int center_rejected,
		const std::vector<QPointF> &used_points, const std::vector<double> &used_radii, const std::vector<QPointF> &rejected_points,
		double pixel_scale, double base_image_px,
		const std::vector<double> &cell_eccentricity = std::vector<double>(), const std::vector<double> &cell_major_angle = std::vector<double>());
	void setWidgetOpacity(double opacity);

	// Run inspection using the provided image. The overlay will call into an ImageInspector
	// implementation and display the result. The ImageInspector lives inside the overlay.
	void runInspection(const preview_image &img);

	// Clear any current inspection results and cancel running inspection.
	void clearInspection();

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	std::vector<double> m_dirs; // 8 directions: N, NE, E, SE, S, SW, W, NW
	double m_center_hfd;
	double m_opacity;

	// per-direction counts
	std::vector<int> m_detected;
	std::vector<int> m_used;
	std::vector<int> m_rejected;

	int m_center_detected;
	int m_center_used;
	int m_center_rejected;

	// Per-star positions in overlay (view) coordinates
	std::vector<QPointF> m_used_points;
	std::vector<double> m_used_radii;
	std::vector<QPointF> m_rejected_points;

	// view scaling (view pixels per image pixel) and base image radius in image pixels
	double m_pixel_scale = 1.0;
	double m_base_image_px = 0.0;

	// per-cell morphology (5x5 grid assumed): eccentricity and major-axis angle (deg)
	std::vector<double> m_cell_eccentricity;
	std::vector<double> m_cell_major_angle;

	// optional pointer to the QGraphicsView so we can map scene->view dynamically
	QGraphicsView *m_viewptr = nullptr;

	// async inspection watcher and sequence token
	QFutureWatcher<InspectionResult> *m_watcher = nullptr;
	std::atomic<uint64_t> m_seq {0};

public:
	// set the view used for mapping scene coordinates to view coordinates
	void setView(QGraphicsView *view) { m_viewptr = view; }
};

#endif // INSPECTION_OVERLAY_H
