#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QFrame>
#include <image_stats.h>
#include <imagepreview.h>
#include <QGraphicsPixmapItem>
#include <QEnterEvent>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class PixmapItem;
class GraphicsView;
class QToolButton;

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

public slots:
	void setText(const QString &txt);
	void setToolTip(const QString &txt);
	void onSetImage(preview_image &im);
	void setImageStats(const ImageStats &stats);

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
	QGraphicsRectItem *m_selection;
	QGraphicsRectItem *m_edge_clipping;
	QGraphicsLineItem *m_ref_x;
	QGraphicsLineItem *m_ref_y;
	QPoint m_selection_p;
	QPoint m_ref_p;
	double m_edge_clipping_v;
	QList<QGraphicsEllipseItem*> m_extra_selections;
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
	void mouseMoved(double x, double y);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;
	void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;

private:
	preview_image m_image;
};

#endif // IMAGEVIEWER_H
