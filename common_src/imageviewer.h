#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QFrame>
#include <image_stats.h>
#include <imagepreview.h>
#include "live_stacker.h"
#include <QGraphicsPixmapItem>
#include <snr_calculator.h>
#include <snr_overlay.h>
#include <image_inspector_overlay.h>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class PixmapItem;
class GraphicsView;
class QToolButton;
class AntialiasedEllipseItem;
class AntialiasedRectItem;

/**
 * @brief ImageViewer displays images and allows basic interaction with it
 */
class ImageViewer : public QFrame {
	Q_OBJECT

public:
	/**
	* ToolBar visibility
	*/
	enum class ToolBarMode {
		Visible,
		Hidden,
		AutoHidden
	};

public:
	explicit ImageViewer(QWidget *parent = nullptr, bool show_prev_next = false, bool show_debayer = true);

	/// Text displayed on the left side of the toolbar
	QString text() const;

	QLabel *getTextLabel() { return m_text_label; };

	/// The currently displayed image
	const preview_image& image() const;

	/// Access to the pixmap so that other tools can add things to it
	const PixmapItem* pixmapItem() const;
	PixmapItem* pixmapItem();

	/// Add a tool to the toolbar
	void addTool(QWidget *tool);

	/// Toolbar visibility
	ToolBarMode toolBarMode() const;
	void setToolBarMode(ToolBarMode mode);

	/// Anti-aliasing
	bool isAntialiasingEnabled() const;
	void enableAntialiasing(bool on = true);
	void setStretch(int level);
	void setDebayer(uint32_t bayer_pat);
	void setBalance(int Balance);
	void showStretchButton(bool show);
	void showZoomButtons(bool show);
	void showStackButton(bool show);
	void setShowStack(bool show);

	QRect getImageFrameRect() const;

public slots:
	void setText(const QString &txt);
	void setToolTip(const QString &txt);
	void onSetImage(preview_image &im);
	void setImageStats(const ImageStats &stats);

	/// Add @p im to the live stack.  If the stack-view toggle is active the
	/// stacked result is shown; otherwise the raw frame is displayed.
	void addToStack(preview_image &im);

	/// Reset the live stack accumulator and switch back to the last-frame view.
	void resetStack();

	void showSelection(bool show);
	void moveSelection(double x, double y);
	void moveResizeSelection(double x, double y, int size);

	void showExtraSelection(bool show);
	void moveResizeExtraSelection(QList<QPointF> &point_list, int size);

	void showReference(bool show);
	void moveReference(double x, double y);
	void centerReference();

	void showWCS(bool show);

	void showEdgeClipping(bool show);
	void resizeEdgeClipping(double edge_clipping);

	void zoomFit();
	void zoomOriginal();
	void zoomIn();
	void zoomOut();
	void mouseAt(double x, double y);
	void mouseRightPressAt(double x, double y, Qt::KeyboardModifiers modifiers);
	void mouseLeftPressAt(double x, double y, Qt::KeyboardModifiers modifiers);
	void mouseLeftDoubleClickAt(double x, double y, Qt::KeyboardModifiers modifiers);

	void stretchNone();
	void stretchSlight();
	void stretchModerate();
	void stretchNormal();
	void stretchHard();

	void debayerAuto();
	void debayerNone();
	void debayerGBRG();
	void debayerGRBG();
	void debayerRGGB();
	void debayerBGGR();

	void onAutoBalance();
	void onNoBalance();

	void onPrevious();
	void onNext();

	void enableSNRMode(bool enable);
	void showSNROverlay(bool show);
	void calculateAndShowSNR(double x, double y);

	// Image inspection
	void runImageInspection();
	void showInspectionOverlay(bool show);
	void updateSNROverlayPosition();
	void updateInspectionOverlayPosition();

signals:
	void imageChanged();
	void setImage(preview_image &im);
	void mouseRightPress(double x, double y, Qt::KeyboardModifiers modifiers);
	void mouseRightPressRADec(double ra, double dec, double telescope_ra, double telescope_dec, Qt::KeyboardModifiers modifiers);
	void zoomChanged(double scale);
	void stretchChanged(int level);
	void debayerChanged(uint32_t bayer_pat);
	void BalanceChanged(int balance);
	void previousRequested();
	void nextRequested();
	void viewerResized();
	void viewerShown();

	/// Emitted whenever the number of stacked frames changes.
	void stackCountChanged(int count);

protected:
	void enterEvent(QEnterEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void showEvent(QShowEvent *event) override;

private:
	void setMatrix();
	void makeToolbar(bool show_prev_next, bool show_debayer);

private:
	void showZoom();
	double m_zoom_level;
	QLabel *m_text_label;
	QLabel *m_pixel_value;
	QLabel *m_image_stats;
	QLabel *m_image_histogram;
	GraphicsView *m_view;
	PixmapItem *m_pixmap;
	AntialiasedRectItem *m_selection;
	AntialiasedRectItem *m_edge_clipping;
	QGraphicsLineItem *m_ref_x;
	QGraphicsLineItem *m_ref_y;
	QPoint m_selection_p;
	QPoint m_ref_p;
	double m_edge_clipping_v;
	QList<AntialiasedEllipseItem*> m_extra_selections;
	bool m_extra_selections_visible;

	QWidget *m_toolbar;
	bool m_fit;
	bool m_selection_visible;
	bool m_ref_visible;
	bool m_show_wcs;
	bool m_edge_clipping_visible;
	ToolBarMode m_bar_mode;
	QToolButton *m_stretch_button;
	QToolButton *m_zoomout_button;
	QToolButton *m_zoomin_button;
	QAction *m_stretch_act[PREVIEW_STRETCH_COUNT];
	QAction *m_debayer_act[DEBAYER_COUNT];
	QAction *m_color_reference_act[COLOR_BALANCE_COUNT];
	SNROverlay *m_snr_overlay;
	ImageInspectorOverlay *m_inspection_overlay;
	QAction *m_inspection_act;
	LiveStacker *m_stacker;
	QToolButton *m_stack_button;
	bool m_show_stack;
	preview_image m_last_image;
	int m_stretch_level;
	uint32_t m_bayer_pat;
	int m_color_balance;

	stretch_config_t currentStretchConfig() const {
		stretch_config_t sc;
		sc.stretch_level = static_cast<uint8_t>(m_stretch_level);
		sc.balance       = static_cast<uint8_t>(m_color_balance);
		sc.bayer_pattern = m_bayer_pat;
		return sc;
	}
	bool m_inspection_overlay_visible;
	AntialiasedEllipseItem *m_snr_star_circle;
	AntialiasedEllipseItem *m_snr_background_inner_ring;
	AntialiasedEllipseItem *m_snr_background_outer_ring;
	bool m_snr_mode_enabled;
	bool m_snr_overlay_visible;
	double m_snr_star_x;
	double m_snr_star_y;
	double m_snr_star_radius;
	double m_snr_background_inner_radius;
	double m_snr_background_outer_radius;
};


class PixmapItem : public QObject, public QGraphicsPixmapItem {
	Q_OBJECT

public:
	PixmapItem(QGraphicsItem *parent = nullptr);
	const preview_image & image() const { return m_image; }

public slots:
	void setImage(preview_image im);

signals:
	void imageChanged(const preview_image &);
	void sizeChanged(int w, int h);
	void mouseRightPress(double x, double y, Qt::KeyboardModifiers);
	void mouseLeftPress(double x, double y, Qt::KeyboardModifiers);
	void mouseLeftDoubleClick(double x, double y, Qt::KeyboardModifiers);
	void mouseMoved(double x, double y);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) override;
	void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;

private:
	preview_image m_image;
	bool m_is_double_click;
};

#endif // IMAGEVIEWER_H
