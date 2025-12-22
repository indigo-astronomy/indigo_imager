#include "imageviewer.h"
#include <cmath>
#include <QGraphicsScene>
#include <QMenu>
#include <QActionGroup>
#include <QGraphicsView>
#include <QGraphicsSceneHoverEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <cmath>

// Graphics View with better mouse events handling
class GraphicsView : public QGraphicsView {
public:
	explicit GraphicsView(ImageViewer *viewer)
		: QGraphicsView()
		, m_viewer(viewer)
	{
		// no antialiasing or filtering, we want to see the exact image content
		setRenderHint(QPainter::Antialiasing, false);
		setDragMode(QGraphicsView::ScrollHandDrag);
		setOptimizationFlags(QGraphicsView::DontSavePainterState);
		setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
		setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // zoom at cursor position
		//setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		//setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setInteractive(true);
		setMouseTracking(true);
	}

protected:
	void wheelEvent(QWheelEvent *event) override {
		if (event->modifiers() == Qt::NoModifier) {
			if (event->delta() > 0)
				m_viewer->zoomIn();
			else if (event->delta() < 0)
				m_viewer->zoomOut();
			event->accept();
		} else {
			QGraphicsView::wheelEvent(event);
		}
	}

	void scrollContentsBy(int dx, int dy) override {
		QGraphicsView::scrollContentsBy(dx, dy);
		// Update SNR overlay position when scrolling
		m_viewer->updateSNROverlayPosition();
	}

	void enterEvent(QEvent *event) override {
		QGraphicsView::enterEvent(event);
		viewport()->setCursor(Qt::CrossCursor);
	}

	void mousePressEvent(QMouseEvent *event) override {
		QGraphicsView::mousePressEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent *event) override {
		QGraphicsView::mouseReleaseEvent(event);
		viewport()->setCursor(Qt::CrossCursor);
	}

private:
	ImageViewer *m_viewer;
};


ImageViewer::ImageViewer(QWidget *parent, bool show_prev_next, bool show_debayer)
	: QFrame(parent)
	, m_zoom_level(0)
	, m_fit(true)
	, m_bar_mode(ToolBarMode::Visible)
	, m_snr_star_x(0)
	, m_snr_star_y(0)
	, m_snr_star_radius(0)
	, m_snr_background_inner_radius(0)
	, m_snr_background_outer_radius(0)
{
	auto scene = new QGraphicsScene(this);
	m_view = new GraphicsView(this);
	m_view->setScene(scene);

	// graphic object holding the image buffer
	m_pixmap = new PixmapItem;
	scene->addItem(m_pixmap);
	connect(m_pixmap, SIGNAL(mouseMoved(double,double)), SLOT(mouseAt(double,double)));
	connect(m_pixmap, SIGNAL(mouseRightPress(double, double, Qt::KeyboardModifiers)), SLOT(mouseRightPressAt(double, double, Qt::KeyboardModifiers)));
	connect(m_pixmap, SIGNAL(mouseLeftPress(double, double, Qt::KeyboardModifiers)), SLOT(mouseLeftPressAt(double, double, Qt::KeyboardModifiers)));
	connect(m_pixmap, SIGNAL(mouseLeftDoubleClick(double, double, Qt::KeyboardModifiers)), SLOT(mouseLeftDoubleClickAt(double, double, Qt::KeyboardModifiers)));

	m_ref_x = new QGraphicsLineItem(25,0,25,50, m_pixmap);
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(QColor(230, 255, 0));
	m_ref_x->setPen(pen);
	m_ref_x->setOpacity(0.5);
	m_ref_x->setVisible(false);

	m_ref_y = new QGraphicsLineItem(0,25,50,25, m_pixmap);
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(QColor(230, 255, 0));
	m_ref_y->setPen(pen);
	m_ref_y->setOpacity(0.5);
	m_ref_y->setVisible(false);

	m_ref_visible = false;
	m_show_wcs = false;

	m_selection = new QGraphicsRectItem(0,0,25,25, m_pixmap);
	m_selection->setBrush(QBrush(Qt::NoBrush));
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(Qt::green);
	m_selection->setPen(pen);
	m_selection->setOpacity(0.7);
	m_selection->setVisible(false);
	m_selection_visible = false;

	m_edge_clipping = new QGraphicsRectItem(0,0,0,0, m_pixmap);
	m_edge_clipping_v = 0;
	m_edge_clipping->setBrush(QBrush(Qt::NoBrush));
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(Qt::red);
	m_edge_clipping->setPen(pen);
	m_edge_clipping->setOpacity(0.7);
	m_edge_clipping->setVisible(false);
	m_edge_clipping_visible = false;

	makeToolbar(show_prev_next, show_debayer);

	auto box = new QVBoxLayout;
	box->setContentsMargins(0,0,0,0);
	box->addWidget(m_toolbar);
	box->addWidget(m_view, 1);
	setLayout(box);

	m_image_histogram = new QLabel(m_view);
	m_image_histogram->setStyleSheet("background-color: rgba(0,0,0,40%); color: rgba(200,200,200,100%);");
	m_image_histogram->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	m_image_histogram->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_image_histogram->move(QPoint(12, 12));
	m_image_histogram->setTextFormat(Qt::RichText);
	m_image_histogram->raise();
	m_image_histogram->setVisible(false);

	m_image_stats = new QLabel(m_view);
	m_image_stats->setStyleSheet("background-color: rgba(0,0,0,40%); color: rgba(200,200,200,100%);");
	m_image_stats->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	m_image_stats->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_image_stats->move(QPoint(12, 128 + 24));
	m_image_stats->setTextFormat(Qt::RichText);
	m_image_stats->raise();
	m_image_stats->setVisible(false);

	m_snr_overlay = new SNROverlay(m_view);
	m_snr_overlay->setVisible(false);

	// Inspection overlay (parent to viewport so mapFromScene coordinates align)
	m_inspection_overlay = new InspectionOverlay(m_view->viewport());
	m_inspection_overlay->setVisible(false);
	connect(m_inspection_overlay, &InspectionOverlay::destroyed, [this](){ m_inspection_overlay = nullptr; });
	m_snr_mode_enabled = false;  // SNR mode disabled by default
	m_snr_overlay_visible = false;

	// Create SNR visualization circles
	QPen snr_pen;
	snr_pen.setCosmetic(true);
	snr_pen.setWidthF(1.5);
	snr_pen.setColor(QColor(0, 255, 0));
	snr_pen.setStyle(Qt::SolidLine);

	m_snr_star_circle = new QGraphicsEllipseItem(0, 0, 20, 20, m_pixmap);
	m_snr_star_circle->setBrush(QBrush(Qt::NoBrush));
	m_snr_star_circle->setPen(snr_pen);
	m_snr_star_circle->setOpacity(0.7);
	m_snr_star_circle->setVisible(false);

	// Inner annulus boundary (yellow)
	snr_pen.setColor(QColor(255, 255, 0));
	m_snr_background_inner_ring = new QGraphicsEllipseItem(0, 0, 40, 40, m_pixmap);
	m_snr_background_inner_ring->setBrush(QBrush(Qt::NoBrush));
	m_snr_background_inner_ring->setPen(snr_pen);
	m_snr_background_inner_ring->setOpacity(0.5);
	m_snr_background_inner_ring->setVisible(false);

	// Outer annulus boundary (yellow)
	snr_pen.setColor(QColor(255, 255, 0));
	m_snr_background_outer_ring = new QGraphicsEllipseItem(0, 0, 60, 60, m_pixmap);
	m_snr_background_outer_ring->setBrush(QBrush(Qt::NoBrush));
	m_snr_background_outer_ring->setPen(snr_pen);
	m_snr_background_outer_ring->setOpacity(0.5);
	m_snr_background_outer_ring->setVisible(false);

	m_extra_selections_visible = false;

	connect(this, &ImageViewer::setImage, this, &ImageViewer::onSetImage);
}

// toolbar with a few quick actions and display information
void ImageViewer::makeToolbar(bool show_prev_next, bool show_debayer) {
	// text and value at pixel
	m_text_label = new QLabel(this);
	m_text_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	m_pixel_value = new QLabel(this);

	auto fit = new QToolButton(this);
	fit->setToolTip(tr("Fit image to window"));
	fit->setIcon(QIcon(":resource/zoom-fit-best.png"));
	fit->setShortcut(QKeySequence(Qt::Key_F));
	connect(fit, SIGNAL(clicked()), SLOT(zoomFit()));

	auto orig = new QToolButton(this);
	orig->setToolTip(tr("Zoom 1:1"));
	orig->setIcon(QIcon(":resource/zoom-original.png"));
	orig->setShortcut(QKeySequence(Qt::Key_1));
	connect(orig, SIGNAL(clicked()), SLOT(zoomOriginal()));

	m_zoomin_button = new QToolButton(this);
	m_zoomin_button->setToolTip(tr("Zoom In"));
	m_zoomin_button->setIcon(QIcon(":resource/zoom-in.png"));
	m_zoomin_button->setShortcut(QKeySequence(Qt::Key_Plus));

	connect(m_zoomin_button, SIGNAL(clicked()), SLOT(zoomIn()));

	m_zoomout_button = new QToolButton(this);
	m_zoomout_button->setToolTip(tr("Zoom Out"));
	m_zoomout_button->setIcon(QIcon(":resource/zoom-out.png"));
	m_zoomout_button->setShortcut(QKeySequence(Qt::Key_Minus));
	connect(m_zoomout_button, SIGNAL(clicked()), SLOT(zoomOut()));

	QMenu *menu = new QMenu("");
	QAction *act;

	QActionGroup *stretch_group = new QActionGroup(this);
	stretch_group->setExclusive(true);
	act = menu->addAction("Stretch: N&one");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::stretchNone);
	stretch_group->addAction(act);
	m_stretch_act[PREVIEW_STRETCH_NONE] = act;

	act = menu->addAction("Stretch: &Slight");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::stretchSlight);
	stretch_group->addAction(act);
	m_stretch_act[PREVIEW_STRETCH_SLIGHT] = act;

	act = menu->addAction("Stretch: &Moderate");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::stretchModerate);
	stretch_group->addAction(act);
	m_stretch_act[PREVIEW_STRETCH_MODERATE] = act;

	act = menu->addAction("Stretch: &Normal");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::stretchNormal);
	stretch_group->addAction(act);
	m_stretch_act[PREVIEW_STRETCH_NORMAL] = act;

	act = menu->addAction("Stretch: &Hard");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::stretchHard);
	stretch_group->addAction(act);
	m_stretch_act[PREVIEW_STRETCH_HARD] = act;

	menu->addSeparator();

	QActionGroup *cb_group = new QActionGroup(this);
	cb_group->setExclusive(true);
	act = menu->addAction("Background Neutralization: O&N");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::onAutoBalance);
	cb_group->addAction(act);
	m_color_reference_act[COLOR_BALANCE_AUTO] = act;

	act = menu->addAction("Background Neutralization: O&FF");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::onNoBalance);
	cb_group->addAction(act);
	m_color_reference_act[COLOR_BALANCE_NONE] = act;

	if (show_debayer) {
		menu->addSeparator();
	}

	QMenu *sub_menu = menu->addMenu("&Debayering");

	if (!show_debayer) {
		sub_menu->menuAction()->setVisible(false);
	}

	QActionGroup *debayer_group = new QActionGroup(this);
	debayer_group->setExclusive(true);
	act = sub_menu->addAction("&Auto");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::debayerAuto);
	debayer_group->addAction(act);
	m_debayer_act[DEBAYER_AUTO] = act;

	act = sub_menu->addAction("Non&e");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::debayerNone);
	debayer_group->addAction(act);
	m_debayer_act[DEBAYER_NONE] = act;

	sub_menu->addSeparator();

	act = sub_menu->addAction("&GBRG");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::debayerGBRG);
	debayer_group->addAction(act);
	m_debayer_act[DEBAYER_GBRG] = act;

	act = sub_menu->addAction("&GRBG");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::debayerGRBG);
	debayer_group->addAction(act);
	m_debayer_act[DEBAYER_GRBG] = act;

	act = sub_menu->addAction("&RGGB");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::debayerRGGB);
	debayer_group->addAction(act);
	m_debayer_act[DEBAYER_RGGB] = act;

	act = sub_menu->addAction("&BGGR");
	act->setCheckable(true);
	connect(act, &QAction::triggered, this, &ImageViewer::debayerBGGR);
	debayer_group->addAction(act);
	m_debayer_act[DEBAYER_BGGR] = act;

	m_stretch_button = new QToolButton(this);
	m_stretch_button->setToolTip(tr("Histogram stretching / Background neutralization / Debayer"));
	m_stretch_button->setIcon(QIcon(":resource/histogram.png"));
	m_stretch_button->setMenu(menu);
	m_stretch_button->setPopupMode(QToolButton::InstantPopup);

	m_toolbar = new QWidget;
	auto box = new QHBoxLayout(m_toolbar);
	m_toolbar->setContentsMargins(0,0,0,0);
	box->setContentsMargins(0,0,0,0);
	if (show_prev_next) {
		auto pn_bar = new QWidget;
		auto pn_box = new QHBoxLayout(pn_bar);
		pn_bar->setContentsMargins(0,0,0,0);
		pn_box->setSpacing(0);
		pn_box->setContentsMargins(0,0,0,0);

		auto prev = new QToolButton(this);
		prev->setToolTip(tr("Previous Image in Folder"));
		prev->setIcon(QIcon(":resource/previous.png"));
		prev->setStyleSheet("QToolButton {border-top-right-radius: 0px; border-bottom-right-radius: 0px;}");
		connect(prev, SIGNAL(clicked()), SLOT(onPrevious()));

		auto next = new QToolButton(this);
		next->setToolTip(tr("Next Image in Folder"));
		next->setIcon(QIcon(":resource/next.png"));
		next->setStyleSheet("QToolButton {border-top-left-radius: 0px; border-bottom-left-radius: 0px;}");
		connect(next, SIGNAL(clicked()), SLOT(onNext()));

		pn_box->addWidget(prev);
		pn_box->addWidget(next);
		box->addWidget(pn_bar);
	}

	box->addWidget(m_text_label);
	box->addStretch(1);
	box->addWidget(m_pixel_value);

	box->addWidget(m_zoomout_button);
	box->addWidget(m_zoomin_button);
	box->addWidget(fit);
	box->addWidget(orig);
	box->addWidget(m_stretch_button);

	// add inspection action to toolbar menu
	QAction *inspectionAct = new QAction(tr("Image Inspection"), this);
	inspectionAct->setCheckable(true);
	connect(inspectionAct, &QAction::triggered, this, [this](bool checked){
		if (checked) {
			runImageInspection();
			showInspectionOverlay(true);
		} else {
			showInspectionOverlay(false);
		}
	});
	menu->addSeparator();
	menu->addAction(inspectionAct);
	m_inspection_act = inspectionAct;
}

void ImageViewer::showStretchButton(bool show) {
	m_stretch_button->setVisible(show);
}

void ImageViewer::showZoomButtons(bool show) {
	m_zoomin_button->setVisible(show);
	m_zoomout_button->setVisible(show);
}

void ImageViewer::setImageStats(const ImageStats &stats) {
	if (stats.channels == 1) {
		QString stats_str = "<b>Statistics</b>";
		if(stats.bitdepth == -32) {
			stats_str += " (float)";
		} else {
			stats_str += " (" + QString::number(stats.bitdepth) + "bit)<br>";
		}
		stats_str += "<table cellspacing=1>";
		stats_str += "<tr><td><b>Min </b></td><td align=right> " + QString::number(stats.grey_red.min) + "</td></tr>";
		stats_str += "<tr><td><b>Max </b></td><td align=right> " + QString::number(stats.grey_red.max) + "</td></tr>";
		stats_str += "<tr><td><b>Mean </b></td><td align=right> " + QString::number(stats.grey_red.mean) + "</td></tr>";
		stats_str += "<tr><td><b>StdDev </b></td><td align=right> " + QString::number(stats.grey_red.stddev) + "</td></tr>";
		stats_str += "<tr><td><b>MAD </b></td><td align=right> "  + QString::number(stats.grey_red.mad) + "</td></tr>";
		stats_str += "</table>";
		m_image_stats->setText(stats_str);
		m_image_stats->adjustSize();
		m_image_stats->setVisible(true);

		QImage hist = makeHistogram(stats);
		m_image_histogram->setPixmap(QPixmap::fromImage(hist));
		m_image_histogram->adjustSize();
		m_image_histogram->setVisible(true);
	} else if (stats.channels == 3) {
		QString stats_str = "<b>Statistics</b> ";
		if(stats.bitdepth == -32) {
			stats_str += " (float)";
		} else {
			stats_str += " (" + QString::number(stats.bitdepth) + "bit)<br>";
		}
		stats_str += "<table cellspacing=1>";
		stats_str += "<tr><td><b>Min </b></td>";
		stats_str += "<td align=right><font color=\"#C05050\"> " + QString::number(stats.grey_red.min) + " </font></td>";
		stats_str += "<td align=right><font color=\"#50C050\"> " + QString::number(stats.green.min) + " </font></td>";
		stats_str += "<td align=right><font color=\"#5050FF\"> " + QString::number(stats.blue.min) + " </font></td>";
		stats_str += "</tr>";

		stats_str += "<tr><td><b>Max </b></td>";
		stats_str += "<td align=right><font color=\"#C05050\"> " + QString::number(stats.grey_red.max) + " </font></td>";
		stats_str += "<td align=right><font color=\"#50C050\"> " + QString::number(stats.green.max) + " </font></td>";
		stats_str += "<td align=right><font color=\"#5050FF\"> " + QString::number(stats.blue.max) + " </font></td>";
		stats_str += "</tr>";

		stats_str += "<tr><td><b>Mean </b></td>";
		stats_str += "<td align=right><font color=\"#C05050\"> " + QString::number(stats.grey_red.mean) + " </font></td>";
		stats_str += "<td align=right><font color=\"#50C050\"> " + QString::number(stats.green.mean) + " </font></td>";
		stats_str += "<td align=right><font color=\"#5050FF\"> " + QString::number(stats.blue.mean) + " </font></td>";
		stats_str += "</tr>";

		stats_str += "<tr><td><b>StdDev </b></td>";
		stats_str += "<td align=right><font color=\"#C05050\"> " + QString::number(stats.grey_red.stddev) + " </font></td>";
		stats_str += "<td align=right><font color=\"#50C050\"> " + QString::number(stats.green.stddev) + " </font></td>";
		stats_str += "<td align=right><font color=\"#5050FF\"> " + QString::number(stats.blue.stddev) + " </font></td>";
		stats_str += "</tr>";

		stats_str += "<tr><td><b>MAD </b></td>";
		stats_str += "<td align=right><font color=\"#C05050\"> " + QString::number(stats.grey_red.mad) + " </font></td>";
		stats_str += "<td align=right><font color=\"#50C050\"> " + QString::number(stats.green.mad) + " </font></td>";
		stats_str += "<td align=right><font color=\"#5050FF\"> " + QString::number(stats.blue.mad) + " </font></td>";
		stats_str += "</tr>";
		stats_str += "</table>";

		m_image_stats->setText(stats_str);
		m_image_stats->adjustSize();
		m_image_stats->setVisible(true);

		QImage hist = makeHistogram(stats);
		m_image_histogram->setPixmap(QPixmap::fromImage(hist));
		m_image_histogram->adjustSize();
		m_image_histogram->setVisible(true);
	} else {
		m_image_stats->setVisible(false);
		m_image_stats->setText("");
		m_image_histogram->setVisible(false);
		m_image_histogram->setText("");
	}
}

QString ImageViewer::text() const {
	return m_text_label->text();
}

void ImageViewer::setText(const QString &txt) {
	m_text_label->setText(txt);
}

void ImageViewer::showZoom() {
	if (m_pixmap->image().isNull()) {
		m_pixel_value->setText(QString());
	} else {
		QString s;
		s = QString("%1%").arg(m_zoom_level, 0, 'f', 0);
		m_pixel_value->setText(s);
	}
}

void ImageViewer::setToolTip(const QString &txt) {
	m_text_label->setToolTip(txt);
}

void ImageViewer::showSelection(bool show) {
	if (show) {
		m_selection_visible = true;
		if (!m_pixmap->pixmap().isNull() && !m_selection_p.isNull()) {
			m_selection->setVisible(true);
		}
	} else {
		m_selection_visible = false;
		m_selection->setVisible(false);
	}
}

void ImageViewer::moveResizeSelection(double x, double y, int size) {
	double cor_x = x - (double)size / 2.0;
	double cor_y = y - (double)size / 2.0;

	m_selection_p.setX(x);
	m_selection_p.setY(y);

	if (!m_pixmap->pixmap().isNull() && ((cor_x < 0) || (cor_y < 0) ||
	    (cor_x > m_pixmap->pixmap().width() - size + 1) ||
	    (cor_y > m_pixmap->pixmap().height() - size + 1))) {
		m_selection->setVisible(false);
		return;
	}
	indigo_debug("%s(): %.2f -> %.2f, %.2f -> %.2f, %d", __FUNCTION__, x, cor_x, y, cor_y, size);
	if (m_selection_p.isNull() || m_pixmap->pixmap().isNull()) {
		m_selection->setVisible(false);
	} else if (m_selection_visible){
		m_selection->setVisible(true);
	}
	m_selection->setRect(0, 0, size, size);
	m_selection->setPos(cor_x, cor_y);
}

void ImageViewer::moveSelection(double x, double y) {
	QRectF br = m_selection->boundingRect();
	double cor_x = x - (br.width() - 1) / 2.0;
	double cor_y = y - (br.height() - 1) / 2.0;

	if (!m_pixmap->pixmap().isNull() && ((cor_x < 0) || (cor_y < 0) ||
	    (cor_x > m_pixmap->pixmap().width() - (int)br.width() + 1) ||
	    (cor_y > m_pixmap->pixmap().height() - (int)br.height() + 1))) {
		return;
	}
	indigo_debug("%s(): %.2f -> %.2f, %.2f -> %.2f, %d", __FUNCTION__, x, cor_x, y, cor_y, (int)br.width()-1);
	m_selection_p.setX(x);
	m_selection_p.setY(y);
	m_selection->setPos(cor_x, cor_y);
}

void ImageViewer::showExtraSelection(bool show) {
	m_extra_selections_visible = show;
	QList<QGraphicsEllipseItem*>::iterator sel;
	for (sel = m_extra_selections.begin(); sel != m_extra_selections.end(); ++sel) {
		if (!m_pixmap->pixmap().isNull()) (*sel)->setVisible(show);
		else (*sel)->setVisible(false);
	}
}

void ImageViewer::moveResizeExtraSelection(QList<QPointF> &point_list, int size) {
	while (!m_extra_selections.isEmpty()) delete m_extra_selections.takeFirst();
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(Qt::green);
	//pen.setColor(QColor(255, 255, 0));
	QList<QPointF>::iterator point;
	for (point = point_list.begin(); point != point_list.end(); ++point) {
		QGraphicsEllipseItem *selection = new QGraphicsEllipseItem(0, 0, size, size, m_pixmap);
		double x = point->x() - size / 2.0;
		double y = point->y() - size / 2.0;
		selection->setRect(0, 0, size, size);
		selection->setPos(x, y);
		selection->setBrush(QBrush(Qt::NoBrush));
		selection->setPen(pen);
		selection->setOpacity(0.7);
		if (x <= size / 2.0 || y <= size / 2.0 || !m_extra_selections_visible || m_pixmap->pixmap().isNull()) {
			selection->setVisible(false);
		} else {
			selection->setVisible(true);
		}
		m_extra_selections.append(selection);
	}
}

void ImageViewer::showReference(bool show) {
	if (show) {
		m_ref_visible = true;
		if (!m_pixmap->pixmap().isNull() && !m_ref_p.isNull()) {
			m_ref_x->setVisible(true);
			m_ref_y->setVisible(true);
		}
	} else {
		m_ref_visible = false;
		m_ref_x->setVisible(false);
		m_ref_y->setVisible(false);
	}
}

void ImageViewer::centerReference() {
	double x_len = m_pixmap->pixmap().width();
	double y_len = m_pixmap->pixmap().height();
	double x = x_len / 2;
	double y = y_len / 2;
	if (m_pixmap->pixmap().isNull()) {
		return;
	}
	indigo_debug("X = %.2f, Y = %.2f, X_len = %.2f, y_len = %.2f", x, y, x_len, y_len);

	m_ref_p.setX(x);
	m_ref_p.setY(y);
	if (m_ref_p.isNull()) {
		m_ref_x->setVisible(false);
		m_ref_y->setVisible(false);
	} else if (m_ref_visible){
		m_ref_x->setVisible(true);
		m_ref_y->setVisible(true);
	}
	m_ref_x->setLine(x, 0, x, y_len);
	m_ref_y->setLine(0, y, x_len, y);
}

void ImageViewer::moveReference(double x, double y) {
	double cor_x = x;
	double cor_y = y;
	double x_len = m_pixmap->pixmap().width();
	double y_len = m_pixmap->pixmap().height();
	if (!m_pixmap->pixmap().isNull() && ((cor_x < 0) || (cor_y < 0) || (cor_x > x_len) || (cor_y > y_len))) {
		return;
	}
	indigo_debug("X = %.2f, Y = %.2f, X_len = %.2f, y_len = %.2f", cor_x, cor_y, x_len, y_len);

	m_ref_p.setX(x);
	m_ref_p.setY(y);
	if (m_ref_p.isNull()) {
		m_ref_x->setVisible(false);
		m_ref_y->setVisible(false);
	} else if (m_ref_visible){
		m_ref_x->setVisible(true);
		m_ref_y->setVisible(true);
	}
	m_ref_x->setLine(cor_x, 0, cor_x, y_len);
	m_ref_y->setLine(0, cor_y, x_len, cor_y);
}

void ImageViewer::showWCS(bool show) {
	m_show_wcs = show;
}

void ImageViewer::showEdgeClipping(bool show) {
	if (show) {
		m_edge_clipping_visible = true;
		if (!m_pixmap->pixmap().isNull()) {
			m_edge_clipping->setVisible(true);
		}
	} else {
		m_edge_clipping_visible = false;
		m_edge_clipping->setVisible(false);
	}
}

void ImageViewer::resizeEdgeClipping(double edge_clipping) {
	m_edge_clipping_v = edge_clipping;

	if (!m_pixmap) return;

	double width = m_pixmap->pixmap().width() - 2 * edge_clipping;
	double height = m_pixmap->pixmap().height() - 2 * edge_clipping;

	indigo_debug("%s(): width = %.2f, height = %.2f, edge_clipping = %.2f", __FUNCTION__, width, height, edge_clipping);

	if (width <= 1 || height <= 1) {
		int ech = m_pixmap->pixmap().height() / 2;
		int ecw = m_pixmap->pixmap().width() / 2;
		edge_clipping = (ecw < ech) ? ecw : ech;
		width = m_pixmap->pixmap().width() - 2 * edge_clipping + 1;
		height = m_pixmap->pixmap().height() - 2 * edge_clipping + 1;
	} else if (m_edge_clipping_visible){
		m_edge_clipping->setVisible(true);
	}
	m_edge_clipping->setRect(edge_clipping, edge_clipping, width, height);
}


const preview_image &ImageViewer::image() const {
	return m_pixmap->image();
}

void ImageViewer::onSetImage(preview_image &im) {
	showSNROverlay(false); // hide SNR overlay when new image is displayed
	m_pixmap->setImage(im);
	if (!m_pixmap->pixmap().isNull()) {
		if (m_selection_visible && !m_selection_p.isNull()) {
			m_selection->setVisible(true);
		} else {
			m_selection->setVisible(false);
		}
		if (m_ref_visible && !m_ref_p.isNull()) {
			m_ref_x->setVisible(true);
			m_ref_y->setVisible(true);
		} else {
			m_ref_x->setVisible(false);
			m_ref_y->setVisible(false);
		}
		if (m_edge_clipping_visible) {
			m_edge_clipping->setVisible(true);
			resizeEdgeClipping(m_edge_clipping_v);
		} else {
			m_edge_clipping->setVisible(false);
		}
		resizeEdgeClipping(m_edge_clipping_v);

		QList<QGraphicsEllipseItem*>::iterator sel;
		for (sel = m_extra_selections.begin(); sel != m_extra_selections.end(); ++sel) {
			//QRectF rect = (*sel)->rect();
			//if (rect.x() > 0 && rect.y() > 0) {
				(*sel)->setVisible(m_extra_selections_visible);
			//} else {
			//	(*sel)->setVisible(false);
			//}
		}
	}
	m_view->scene()->setSceneRect(0, 0, im.width(), im.height());

	if (m_fit) zoomFit();

	emit imageChanged();
}

const PixmapItem *ImageViewer::pixmapItem() const {
	return m_pixmap;
}

PixmapItem *ImageViewer::pixmapItem() {
	return m_pixmap;
}

ImageViewer::ToolBarMode ImageViewer::toolBarMode() const {
	return m_bar_mode;
}

void ImageViewer::setToolBarMode(ToolBarMode mode) {
	m_bar_mode = mode;
	if (mode == ToolBarMode::Hidden)
		m_toolbar->hide();
	else if (mode == ToolBarMode::Visible)
		m_toolbar->show();
	else
		m_toolbar->setVisible(underMouse());
}

bool ImageViewer::isAntialiasingEnabled() const {
	return m_view->renderHints() & QPainter::Antialiasing;
}

void ImageViewer::enableAntialiasing(bool on) {
	m_view->setRenderHint(QPainter::Antialiasing, on);
	if (on) {
		m_pixmap->setTransformationMode(Qt::SmoothTransformation);
	} else {
		m_pixmap->setTransformationMode(Qt::FastTransformation);
	}
}

void ImageViewer::addTool(QWidget *tool) {
	m_toolbar->layout()->addWidget(tool);
}

void ImageViewer::setMatrix() {
	qreal scale = m_zoom_level / 100.0;

	QTransform matrix;
	matrix.scale(scale, scale);

	m_view->setTransform(matrix);
	emit zoomChanged(m_view->transform().m11());
}

void ImageViewer::zoomFit() {
	m_view->fitInView(m_pixmap, Qt::KeepAspectRatio);
	m_zoom_level = (100.0 * m_view->transform().m11());
	showZoom();
	indigo_debug("Zoom FIT = %.2f", m_zoom_level);
	m_fit = true;
	emit zoomChanged(m_view->transform().m11());
	updateSNROverlayPosition();  // Update SNR overlay position after zoom
}

void ImageViewer::zoomOriginal() {
	m_zoom_level = 100;
	showZoom();
	indigo_debug("Zoom 1:1 = %.2f", m_zoom_level);
	m_fit = false;
	setMatrix();
	updateSNROverlayPosition();  // Update SNR overlay position after zoom
}

void ImageViewer::zoomIn() {
	if (!(m_pixmap && m_pixmap->image().valid(1,1))) return;

	if (m_zoom_level >= 1000) {
		m_zoom_level = floor(m_zoom_level / 500.0) * 500 + 500;
	} else if (m_zoom_level >= 100) {
		m_zoom_level = floor(m_zoom_level / 50.0) * 50 + 50;
	} else if (m_zoom_level >= 10) {
		m_zoom_level = floor(m_zoom_level / 10.0) * 10 + 10;
	} else if (m_zoom_level >= 1) {
		m_zoom_level = floor(m_zoom_level) + 1;
	} else {
		m_zoom_level = 1;
	}
	if (m_zoom_level > 5000) {
		m_zoom_level = 5000;
	}
	showZoom();
	indigo_debug("Zoom IN = %.2f", m_zoom_level);
	m_fit = false;
	setMatrix();
	updateSNROverlayPosition();  // Update SNR overlay position after zoom
}

void ImageViewer::zoomOut() {
	if (!(m_pixmap && m_pixmap->image().valid(1,1))) return;

	if (m_zoom_level > 1000) {
		m_zoom_level = ceil(m_zoom_level / 500.0) * 500 - 500;
	} else if (m_zoom_level > 100) {
		m_zoom_level = ceil(m_zoom_level / 50.0) * 50 - 50;
	} else if (m_zoom_level > 10) {
		m_zoom_level = ceil(m_zoom_level / 10.0) * 10 - 10;
	} else if (m_zoom_level > 1) {
		m_zoom_level = ceil(m_zoom_level) - 1;
	} else {
		m_zoom_level = 1;
	}
	// Do not zoom out bellow zoom fit or 100% if zoom fit is bigger than 100%
	QRectF rect = m_view->viewport()->geometry();
	double scale_x = rect.width() / m_pixmap->image().width() * 100;
	double scale_y = rect.height() / m_pixmap->image().height() * 100;
	double zoom_min = (scale_x < scale_y) ? scale_x : scale_y;
	zoom_min = (zoom_min < 100) ? zoom_min : 100;
	m_zoom_level = (zoom_min > m_zoom_level) ? zoom_min : m_zoom_level;
	showZoom();
	indigo_debug("Zoom OUT = %.2f (fit = %.2f)", m_zoom_level, zoom_min);
	m_fit = false;
	setMatrix();
	updateSNROverlayPosition();  // Update SNR overlay position after zoom
}

void ImageViewer::mouseAt(double x, double y) {
	if (m_pixmap && m_pixmap->image().valid(x,y)) {
		double r=0, g=0, b=0;
		double ra, dec;
		int pix_format = m_pixmap->image().pixel_value(x, y, r, g, b);
		int res = m_pixmap->image().wcs_data(x, y, &ra, &dec);
		QString s;
		if (res != -1 && m_show_wcs) {
			s = QString("%1% [%2, %3] (%4, %5) ")
				.arg(m_zoom_level, 0, 'f', 0)
				.arg(x, 5, 'f', 1)
				.arg(y, 5, 'f', 1)
				.arg(indigo_dtos(ra / 15, "%dh %02d' %04.1f\""))
				.arg(indigo_dtos(dec, "%+d° %02d' %04.1f\""));
		} else {
			if (pix_format == PIX_FMT_INDEX) {
				s = QString("%1% [%2, %3]")
					.arg(m_zoom_level, 0, 'f', 0)
					.arg(x, 5, 'f', 1)
					.arg(y, 5, 'f', 1);

			} else if (pix_format == PIX_FMT_F32 || pix_format == PIX_FMT_RGBF){
				if (g == -1) {
					s = QString("%1% [%2, %3] (%4)")
						.arg(m_zoom_level, 0, 'f', 0)
						.arg(x, 5, 'f', 1)
						.arg(y, 5, 'f', 1)
						.arg(r, 0, 'f', 6);
				} else {
					s = QString("%1% [%2, %3] (%4, %5, %6)")
						.arg(m_zoom_level, 0, 'f', 0)
						.arg(x, 5, 'f', 1)
						.arg(y, 5, 'f', 1)
						.arg(r, 0, 'f', 6)
						.arg(g, 0, 'f', 6)
						.arg(b, 0, 'f', 6);
				}
			} else {
				if (g == -1) {
					s = QString("%1% [%2, %3] (%4)")
						.arg(m_zoom_level, 0, 'f', 0)
						.arg(x, 5, 'f', 1)
						.arg(y, 5, 'f', 1)
						.arg(r, 5, 'f', 0);
				} else {
					s = QString("%1% [%2, %3] (%4, %5, %6)")
						.arg(m_zoom_level, 0, 'f', 0)
						.arg(x, 5, 'f', 1)
						.arg(y, 5, 'f', 1)
						.arg(r, 5, 'f', 0)
						.arg(g, 5, 'f', 0)
						.arg(b, 5, 'f', 0);
				}
			}
		}
		m_pixel_value->setText(s);
	} else {
		showZoom();
	}
}

void ImageViewer::showSNROverlay(bool show) {
	m_snr_overlay_visible = show;
	m_snr_overlay->setVisible(show);
	m_snr_star_circle->setVisible(show);
	m_snr_background_inner_ring->setVisible(show);
	m_snr_background_outer_ring->setVisible(show);
}

void ImageViewer::showInspectionOverlay(bool show) {
	m_inspection_overlay_visible = show;
	if (m_inspection_overlay) {
		m_inspection_overlay->setVisible(show);
		m_inspection_overlay->update();
	}
}

// Helper: sigma clipping
static double average_without_outliers(std::vector<double> &v) {
	if (v.empty()) return 0.0;
	// iterative 2-sigma clipping
	for (int iter = 0; iter < 3; ++iter) {
		double sum = 0, sumsq = 0;
		for (double x : v) { sum += x; sumsq += x*x; }
		double mean = sum / v.size();
		double var = sumsq / v.size() - mean*mean;
		double sd = var > 0 ? std::sqrt(var) : 0;
		std::vector<double> nv;
		for (double x : v) if (std::fabs(x - mean) <= 2.0 * sd) nv.push_back(x);
		if (nv.size() == v.size()) break;
		if (nv.empty()) return mean;
		v.swap(nv);
	}
	double sum = 0;
	for (double x : v) sum += x;
	return v.empty() ? 0.0 : sum / v.size();
}

void ImageViewer::runImageInspection() {
	if (!m_pixmap) return;
	const preview_image &img = m_pixmap->image();
	if (img.m_raw_data == nullptr) return;

	int width = img.width();
	int height = img.height();
	if (width <= 0 || height <= 0) return;

	// 5x5 grid
	int gx = 5, gy = 5;
	int cell_w = width / gx;
	int cell_h = height / gy;

	// store average HFD per cell
	std::vector<double> cell_hfd(gx * gy, 0.0);
	// per-cell counts
	std::vector<int> cell_detected(gx * gy, 0);
	std::vector<int> cell_used(gx * gy, 0); // after clipping
	std::vector<int> cell_rejected(gx * gy, 0);

	// (old sampling step removed; using peak-finding instead)
	// SNR threshold for including a star in HFD calculation (lower to include fainter stars)
	const double INSPECTION_SNR_THRESHOLD = 8.0;

	// collect per-star viewport positions for overlay
	std::vector<QPointF> inspection_used_points;
	std::vector<double> inspection_used_radii;
	std::vector<QPointF> inspection_rejected_points;
	const bool INSPECTION_DEBUG = true;
	// per-cell used candidate storage to allow global deduplication later
	std::vector<std::vector<std::tuple<double,double,double,double>>> per_cell_used; // x,y,snr,star_radius
	per_cell_used.resize(gx * gy);
	// collect every unique candidate globally so we can recompute per-cell detected counts from the original owner cell
	struct UniqueCand { double x; double y; double snr; double hfd; int cell; };
	std::vector<UniqueCand> all_unique_candidates;

	// Only inspect the 9 target regions: 4 corners, 4 mid-edges, and center
	std::vector<bool> isTarget(gx * gy, false);
	auto mark = [&](int tx, int ty){ if (tx >= 0 && tx < gx && ty >= 0 && ty < gy) isTarget[ty * gx + tx] = true; };
	// corners
	mark(0,0); mark(gx-1,0); mark(gx-1,gy-1); mark(0,gy-1);
	// mid-edges and center
	mark(gx/2, 0); mark(gx-1, gy/2); mark(gx/2, gy-1); mark(0, gy/2); mark(gx/2, gy/2);

	for (int cy = 0; cy < gy; ++cy) {
		for (int cx = 0; cx < gx; ++cx) {
			// skip non-target cells (leave zeros)
			if (!isTarget[cy * gx + cx]) {
				cell_hfd[cy * gx + cx] = 0.0;
				cell_detected[cy * gx + cx] = 0;
				cell_used[cy * gx + cx] = 0;
				cell_rejected[cy * gx + cx] = 0;
				continue;
			}
			struct Candidate { double x; double y; double hfd; double star_radius; double snr; };
			std::vector<Candidate> candidates;
			int x0 = cx * cell_w;
			int y0 = cy * cell_h;
			int x1 = (cx == gx-1) ? width-1 : x0 + cell_w - 1;
			int y1 = (cy == gy-1) ? height-1 : y0 + cell_h - 1;

			// Expand search bounds slightly so stars near cell edges are not missed
			const int MARGIN_SEARCH = 8;
			const int MARGIN_CENTROID = 8; // allow centroid to be slightly outside nominal cell
			int sx0 = std::max(0, x0 - MARGIN_SEARCH);
			int sy0 = std::max(0, y0 - MARGIN_SEARCH);
			int sx1 = std::min(width - 1, x1 + MARGIN_SEARCH);
			int sy1 = std::min(height - 1, y1 + MARGIN_SEARCH);

			// Peak-finding pass: compute mean/stddev over expanded search area
			double cell_sum = 0.0, cell_sumsq = 0.0;
			int cell_count = 0;
			for (int yy = sy0; yy <= sy1; ++yy) {
				for (int xx = sx0; xx <= sx1; ++xx) {
					double rval = 0, gval = 0, bval = 0;
					img.pixel_value(xx, yy, rval, gval, bval);
					cell_sum += rval;
					cell_sumsq += rval * rval;
					++cell_count;
				}
			}
			double cell_mean = (cell_count > 0) ? (cell_sum / cell_count) : 0.0;
			double cell_var = (cell_count > 0) ? (cell_sumsq / cell_count - cell_mean * cell_mean) : 0.0;
			double cell_sd = cell_var > 0 ? std::sqrt(cell_var) : 0.0;
			double peak_threshold = cell_mean + 1.0 * cell_sd; // slightly more sensitive

			// scan expanded search area
			int scan_step = 1;
			for (int yy = sy0; yy <= sy1; yy += scan_step) {
				for (int xx = sx0; xx <= sx1; xx += scan_step) {
					double center_val = 0, gv = 0, bv = 0;
					img.pixel_value(xx, yy, center_val, gv, bv);
					if (center_val <= peak_threshold) continue;
					bool is_local_max = true;
					for (int ny = yy - 1; ny <= yy + 1; ++ny) {
						for (int nx = xx - 1; nx <= xx + 1; ++nx) {
							if (nx == xx && ny == yy) continue;
							if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
							double nval = 0, ng = 0, nb = 0;
							img.pixel_value(nx, ny, nval, ng, nb);
							if (nval > center_val) { is_local_max = false; break; }
						}
						if (!is_local_max) break;
					}
					if (!is_local_max) continue;

					// call calculateSNR at peak position to refine and validate
					SNRResult r = calculateSNR(reinterpret_cast<const uint8_t*>(img.m_raw_data), width, height, img.m_pix_format, xx, yy);
					if (r.valid && !r.is_saturated) {
						// Accept candidate if centroid falls within the expanded search region for this target cell
						int cx_i = static_cast<int>(std::round(r.star_x));
						int cy_i = static_cast<int>(std::round(r.star_y));
						if (cx_i >= x0 - MARGIN_CENTROID && cx_i <= x1 + MARGIN_CENTROID && cy_i >= y0 - MARGIN_CENTROID && cy_i <= y1 + MARGIN_CENTROID) {
							candidates.push_back({r.star_x, r.star_y, r.hfd, r.star_radius, r.snr});
						} else {
							if (INSPECTION_DEBUG && r.snr >= 8.0) {
								fprintf(stderr, "Inspection: skipped centroid outside cell (%d,%d) peak=(%d,%d) centroid=(%.2f,%.2f) snr=%.2f hfd=%.2f bounds x[%d..%d] y[%d..%d]\n",
										cx, cy, xx, yy, r.star_x, r.star_y, r.snr, r.hfd, x0, x1, y0, y1);
							}
						}
					} else {
						if (INSPECTION_DEBUG && center_val > peak_threshold) {
							fprintf(stderr, "Inspection: calculateSNR invalid at peak=(%d,%d) center_val=%.2f r.valid=%d r.is_saturated=%d r.snr=%.2f\n",
									xx, yy, center_val, r.valid ? 1 : 0, r.is_saturated ? 1 : 0, r.snr);
						}
					}
				}
			}

			// Deduplicate nearby detections: keep highest-SNR candidate within threshold
			std::vector<Candidate> unique;
			const double DUPLICATE_RADIUS = 5.0; // pixels
			for (const Candidate &c : candidates) {
				// only consider candidates above SNR threshold and with positive HFD
				if (!(c.snr > INSPECTION_SNR_THRESHOLD && c.hfd > 0)) continue;
				bool replaced = false;
				for (Candidate &u : unique) {
					double dx = c.x - u.x;
					double dy = c.y - u.y;
					double dist = std::sqrt(dx*dx + dy*dy);
					if (dist <= DUPLICATE_RADIUS) {
						// keep the candidate with higher SNR
						if (c.snr > u.snr) u = c;
						replaced = true;
						break;
					}
				}
				if (!replaced) unique.push_back(c);
			}

			// Debug: report candidates removed by duplicate filtering
			if (INSPECTION_DEBUG) {
				for (const Candidate &c : candidates) {
					if (!(c.snr > INSPECTION_SNR_THRESHOLD && c.hfd > 0)) continue;
					bool found_in_unique = false;
					for (const Candidate &u : unique) {
						double dx = c.x - u.x;
						double dy = c.y - u.y;
						if (std::sqrt(dx*dx + dy*dy) <= DUPLICATE_RADIUS) { found_in_unique = true; break; }
					}
					if (!found_in_unique) {
						fprintf(stderr, "Inspection: candidate (%.2f,%.2f) snr=%.2f removed by duplicate in cell(%d,%d)\n", c.x, c.y, c.snr, cx, cy);
					}
				}
			}

			// Fallback: if too few unique detections in this target cell, do a denser sampling inside the original cell
			const int MIN_STARS_PER_CELL = 6;
			const double FALLBACK_SNR = 6.0;
			if (unique.size() < static_cast<size_t>(MIN_STARS_PER_CELL)) {
				// sample a coarse grid inside the original cell (not expanded) and validate with calculateSNR
				int fallback_step = 4; // every 4 pixels
				for (int yy = y0; yy <= y1; yy += fallback_step) {
					for (int xx = x0; xx <= x1; xx += fallback_step) {
						SNRResult r = calculateSNR(reinterpret_cast<const uint8_t*>(img.m_raw_data), width, height, img.m_pix_format, xx, yy);
						if (!r.valid || r.is_saturated) continue;
						if (!(r.snr > FALLBACK_SNR && r.hfd > 0)) continue;
						int cx_i = static_cast<int>(std::round(r.star_x));
						int cy_i = static_cast<int>(std::round(r.star_y));
						const int MARGIN_CENTROID_FALLBACK = 12;
						if (cx_i < x0 - MARGIN_CENTROID_FALLBACK || cx_i > x1 + MARGIN_CENTROID_FALLBACK || cy_i < y0 - MARGIN_CENTROID_FALLBACK || cy_i > y1 + MARGIN_CENTROID_FALLBACK) continue;
						// check duplicates against unique
						bool dup = false;
						for (Candidate &u : unique) {
							double dx = r.star_x - u.x;
							double dy = r.star_y - u.y;
							double dist = std::sqrt(dx*dx + dy*dy);
							if (dist <= DUPLICATE_RADIUS) {
								// keep the higher SNR one
								if (r.snr > u.snr) u = {r.star_x, r.star_y, r.hfd, r.star_radius, r.snr};
								dup = true;
								break;
							}
						}
						if (!dup) unique.push_back({r.star_x, r.star_y, r.hfd, r.star_radius, r.snr});
					}
				}
			}

			// Build hfds vector from unique detections and keep indices
			std::vector<std::pair<double,int>> hv; // hfd, index into unique
			for (size_t i = 0; i < unique.size(); ++i) hv.emplace_back(unique[i].hfd, static_cast<int>(i));

			// sigma-clipping with indices to determine which candidates are used
			double avg = 0.0;
			int used_after = 0;
			int rejected = 0;
			std::vector<int> used_indices;
			if (!hv.empty()) {
				std::vector<std::pair<double,int>> v = hv;
				for (int iter = 0; iter < 3; ++iter) {
					double sum = 0, sumsq = 0;
					for (auto &p : v) { sum += p.first; sumsq += p.first * p.first; }
					double mean = sum / v.size();
					double var = sumsq / v.size() - mean * mean;
					double sd = var > 0 ? std::sqrt(var) : 0;
					std::vector<std::pair<double,int>> nv;
					for (auto &p : v) if (std::fabs(p.first - mean) <= 2.0 * sd) nv.push_back(p);
					if (nv.size() == v.size()) break;
					if (nv.empty()) { break; }
					v.swap(nv);
				}
				// final used indices
				for (auto &p : v) used_indices.push_back(p.second);
				// compute avg from used values
				double sum = 0;
				for (auto &p : v) sum += p.first;
				avg = v.empty() ? 0.0 : sum / v.size();
				used_after = static_cast<int>(v.size());
				rejected = static_cast<int>(hv.size()) - used_after;
			}

			// store used/rejected candidates per cell for later global deduplication
			for (size_t i = 0; i < unique.size(); ++i) {
				bool is_used = false;
				for (int idx : used_indices) if (static_cast<size_t>(idx) == i) { is_used = true; break; }
				QPointF centerScene(unique[i].x, unique[i].y);
				double rscene = unique[i].star_radius;
				if (is_used) {
					per_cell_used[cy * gx + cx].emplace_back(unique[i].x, unique[i].y, unique[i].snr, rscene);
				} else {
					// keep rejected as before (map now)
					QPointF centerView = m_view->mapFromScene(centerScene);
					inspection_rejected_points.push_back(centerView);
				}
				// record this unique candidate for global detected count recomputation (store original owner cell)
				all_unique_candidates.push_back({unique[i].x, unique[i].y, unique[i].snr, unique[i].hfd, cy * gx + cx});
			}
			cell_hfd[cy * gx + cx] = avg;
			// Sanity clamp counts to avoid pathological values
			size_t nd = unique.size();
			if (nd > 5000) {
				fprintf(stderr, "Inspection: large unique count in cell (%d,%d): %zu - clamping\n", cx, cy, nd);
			}
			int detected_count = static_cast<int>(std::min(nd, static_cast<size_t>(9999)));
			cell_detected[cy * gx + cx] = detected_count;
			cell_used[cy * gx + cx] = std::min(used_after, 9999);
			cell_rejected[cy * gx + cx] = std::min(rejected, 9999);
		}
	}

	// Before extracting directions, perform global deduplication of used candidates so counts match drawn markers
	struct GlobalEntry { double x; double y; double snr; double star_radius; int cell; };
	std::vector<GlobalEntry> global_used;
	const double GLOBAL_DUP_RADIUS = 5.0;
	for (int ci = 0; ci < gx * gy; ++ci) {
		for (auto &t : per_cell_used[ci]) {
			double x = std::get<0>(t);
			double y = std::get<1>(t);
			double snr = std::get<2>(t);
			double sr = std::get<3>(t);
			bool merged = false;
			for (auto &g : global_used) {
				double dx = g.x - x;
				double dy = g.y - y;
				if (std::sqrt(dx*dx + dy*dy) <= GLOBAL_DUP_RADIUS) {
					// keep highest SNR but do NOT change original owner cell
					if (snr > g.snr) {
						g.x = x; g.y = y; g.snr = snr; g.star_radius = sr;
						// preserve g.cell (original owner)
					}
					merged = true;
					break;
				}
			}
			if (!merged) global_used.push_back({x,y,snr,sr,ci});
		}
	}

	// Debug: print all deduplicated used candidates with mapped cell and direction
	if (INSPECTION_DEBUG) {
		for (size_t i = 0; i < global_used.size(); ++i) {
			const auto &g = global_used[i];
			int cell_x = static_cast<int>(std::floor(g.x / cell_w));
			int cell_y = static_cast<int>(std::floor(g.y / cell_h));
			cell_x = std::max(0, std::min(gx-1, cell_x));
			cell_y = std::max(0, std::min(gy-1, cell_y));
			int dir = -1;
			if (cell_x == 2 && cell_y == 0) dir = 0; // N
			else if (cell_x == 4 && cell_y == 0) dir = 1; // NE
			else if (cell_x == 4 && cell_y == 2) dir = 2; // E
			else if (cell_x == 4 && cell_y == 4) dir = 3; // SE
			else if (cell_x == 2 && cell_y == 4) dir = 4; // S
			else if (cell_x == 0 && cell_y == 4) dir = 5; // SW
			else if (cell_x == 0 && cell_y == 2) dir = 6; // W
			else if (cell_x == 0 && cell_y == 0) dir = 7; // NW
			fprintf(stderr, "Inspection: used[%zu] scene=(%.2f,%.2f) snr=%.2f mapped_cell=(%d,%d) dir=%d\n", i, g.x, g.y, g.snr, cell_x, cell_y, dir);
		}
	}

	// rebuild inspection_used_points and used counts per cell from deduplicated global_used
	// reset used counts
	std::fill(cell_used.begin(), cell_used.end(), 0);
	inspection_used_points.clear();
	inspection_used_radii.clear();
	for (const auto &g : global_used) {
		QPointF centerScene(g.x, g.y);
		QPointF edgeScene(g.x + g.star_radius, g.y);
		QPointF centerView = m_view->mapFromScene(centerScene);
		QPointF edgeView = m_view->mapFromScene(edgeScene);
		double radiusView = std::fabs(edgeView.x() - centerView.x());
		inspection_used_points.push_back(centerView);
		inspection_used_radii.push_back(std::max(1.5, radiusView));
		// increment used count for the original owner cell (preserve per-cell ownership)
		int cell_index = g.cell;
		if (cell_index >= 0 && cell_index < gx * gy) cell_used[cell_index]++;
	}

	// recompute rejected counts per cell as detected - used (clamp >= 0)
	// recompute detected counts from actual centroids of unique candidates so D matches markers' locations
	std::vector<int> cell_detected_final(gx * gy, 0);
	for (const auto &u : all_unique_candidates) {
		// use stored original owner cell for detected counts instead of recomputing from centroid
		int owner = u.cell;
		if (owner < 0 || owner >= gx * gy) continue;
		cell_detected_final[owner]++;
	}
	for (int i = 0; i < gx * gy; ++i) {
		cell_detected[i] = cell_detected_final[i];
		cell_rejected[i] = std::max(0, cell_detected[i] - cell_used[i]);
	}

	// Rebuild rejected points from unique candidates that did NOT survive global dedup
	inspection_rejected_points.clear();
	for (const auto &u : all_unique_candidates) {
		bool matched = false;
		for (const auto &g : global_used) {
			double dx = g.x - u.x;
			double dy = g.y - u.y;
			double dist = std::sqrt(dx*dx + dy*dy);
			if (dist <= GLOBAL_DUP_RADIUS) { matched = true; break; }
		}
		if (!matched) {
			QPointF centerView = m_view->mapFromScene(QPointF(u.x, u.y));
			inspection_rejected_points.push_back(centerView);
		}
	}

	// extract 8 directions: N, NE, E, SE, S, SW, W, NW using cells
	// map grid indices: (0,0) top-left
	auto getCell = [&](int gx_i, int gy_i)->double{
		if (gx_i < 0 || gx_i >= gx || gy_i < 0 || gy_i >= gy) return 0.0;
		return cell_hfd[gy_i * gx + gx_i];
	};

	// positions: N = center top (2,0), NE=(4,0), E=(4,2), SE=(4,4), S=(2,4), SW=(0,4), W=(0,2), NW=(0,0)
	std::vector<double> dirs(8, 0.0);
	dirs[0] = getCell(2,0); // N
	dirs[1] = getCell(4,0); // NE
	dirs[2] = getCell(4,2); // E
	dirs[3] = getCell(4,4); // SE
	dirs[4] = getCell(2,4); // S
	dirs[5] = getCell(0,4); // SW
	dirs[6] = getCell(0,2); // W
	dirs[7] = getCell(0,0); // NW

	double center_hfd = getCell(2,2);

	if (m_inspection_overlay) {
		// map counts to 8 directions and center
		std::vector<int> detected_dirs(8,0), used_dirs(8,0), rejected_dirs(8,0);

		auto getCount = [&](const std::vector<int> &vec, int gx_i, int gy_i)->int{
			if (gx_i < 0 || gx_i >= gx || gy_i < 0 || gy_i >= gy) return 0;
			return vec[gy_i * gx + gx_i];
		};
		detected_dirs[0] = getCount(cell_detected, 2,0);
		detected_dirs[1] = getCount(cell_detected, 4,0);
		detected_dirs[2] = getCount(cell_detected, 4,2);
		detected_dirs[3] = getCount(cell_detected, 4,4);
		detected_dirs[4] = getCount(cell_detected, 2,4);
		detected_dirs[5] = getCount(cell_detected, 0,4);
		detected_dirs[6] = getCount(cell_detected, 0,2);
		detected_dirs[7] = getCount(cell_detected, 0,0);

		used_dirs[0] = getCount(cell_used, 2,0);
		used_dirs[1] = getCount(cell_used, 4,0);
		used_dirs[2] = getCount(cell_used, 4,2);
		used_dirs[3] = getCount(cell_used, 4,4);
		used_dirs[4] = getCount(cell_used, 2,4);
		used_dirs[5] = getCount(cell_used, 0,4);
		used_dirs[6] = getCount(cell_used, 0,2);
		used_dirs[7] = getCount(cell_used, 0,0);

		rejected_dirs[0] = getCount(cell_rejected, 2,0);
		rejected_dirs[1] = getCount(cell_rejected, 4,0);
		rejected_dirs[2] = getCount(cell_rejected, 4,2);
		rejected_dirs[3] = getCount(cell_rejected, 4,4);
		rejected_dirs[4] = getCount(cell_rejected, 2,4);
		rejected_dirs[5] = getCount(cell_rejected, 0,4);
		rejected_dirs[6] = getCount(cell_rejected, 0,2);
		rejected_dirs[7] = getCount(cell_rejected, 0,0);

		int center_detected = getCount(cell_detected, 2,2);
		int center_used = getCount(cell_used, 2,2);
		int center_rejected = getCount(cell_rejected, 2,2);

		m_inspection_overlay->setInspectionResult(dirs, center_hfd, detected_dirs, used_dirs, rejected_dirs,
			center_detected, center_used, center_rejected, inspection_used_points, inspection_used_radii, inspection_rejected_points);
		// position overlay over view
		m_inspection_overlay->setGeometry(m_view->viewport()->rect());
		m_inspection_overlay->raise();
	}
}

void ImageViewer::calculateAndShowSNR(double x, double y) {
	const preview_image &img = m_pixmap->image();
	if (!img.valid(x, y) || !img.m_raw_data) {
		return;
	}

	SNRResult result = calculateSNR(
		reinterpret_cast<const uint8_t*>(img.m_raw_data),
		img.width(),
		img.height(),
		img.m_pix_format,
		x, y
	);

	if (result.valid) {
		// Store star position for scroll updates
		m_snr_star_x = result.star_x;
		m_snr_star_y = result.star_y;
		m_snr_star_radius = result.star_radius;
		m_snr_background_inner_radius = result.background_inner_radius;
		m_snr_background_outer_radius = result.background_outer_radius;

		// Set the result first so the overlay gets sized properly
		m_snr_overlay->setSNRResult(result);
		showSNROverlay(true);
		m_snr_overlay->updateGeometry();
		updateSNROverlayPosition();

		m_snr_overlay->raise();

		// Show SNR visualization circles
		m_snr_star_circle->setVisible(true);
		m_snr_background_inner_ring->setVisible(true);
		m_snr_background_outer_ring->setVisible(true);

		// Draw aperture star circle
		double diameter = result.star_radius * 2;
		m_snr_star_circle->setRect(0, 0, diameter, diameter);
		m_snr_star_circle->setPos(
			result.star_x - result.star_radius,
			result.star_y - result.star_radius
		);

		// Draw background annulus inner boundary
		double bg_inner_diameter = result.background_inner_radius * 2.0;
		m_snr_background_inner_ring->setRect(0, 0, bg_inner_diameter, bg_inner_diameter);
		m_snr_background_inner_ring->setPos(
			result.star_x - result.background_inner_radius,
			result.star_y - result.background_inner_radius
		);

		// Draw background annulus outer boundary
		double bg_outer_diameter = result.background_outer_radius * 2.0;
		m_snr_background_outer_ring->setRect(0, 0, bg_outer_diameter, bg_outer_diameter);
		m_snr_background_outer_ring->setPos(
			result.star_x - result.background_outer_radius,
			result.star_y - result.background_outer_radius
		);
	} else {
		m_snr_star_radius = 0;
		m_snr_background_inner_radius = 0;
		m_snr_background_outer_radius = 0;

		// Convert click position from scene to view coordinates
		QPointF click_view = m_view->mapFromScene(QPointF(x, y));

		QPoint view_pos(click_view.x() + 10, click_view.y() + 10);
		m_snr_overlay->move(view_pos);

		m_snr_overlay->setSNRResult(result);
		m_snr_overlay->setVisible(true);
		m_snr_overlay->raise();
		m_snr_star_circle->setVisible(false);
		m_snr_background_inner_ring->setVisible(false);
		m_snr_background_outer_ring->setVisible(false);

		m_snr_overlay_visible = true;
	}
}

void ImageViewer::updateSNROverlayPosition() {
	if (!m_snr_overlay_visible) {
		return;
	}

	if (m_snr_star_radius <= 0) {
		return;
	}

	// Ensure the widget has correct size
	m_snr_overlay->adjustSize();
	int overlay_width = m_snr_overlay->width();
	int overlay_height = m_snr_overlay->height();

	// Use actual background outer radius from SNR calculation
	double annulus_radius_scene = m_snr_background_outer_radius;

	// Convert annulus radius from scene to view coordinates (accounts for zoom)
	QPointF star_view = m_view->mapFromScene(QPointF(m_snr_star_x, m_snr_star_y));
	QPointF annulus_edge_view = m_view->mapFromScene(QPointF(m_snr_star_x + annulus_radius_scene, m_snr_star_y + annulus_radius_scene));
	double annulus_radius_view = annulus_edge_view.x() - star_view.x(); // View space distance

	QPoint view_pos(star_view.x() + annulus_radius_view, star_view.y() + annulus_radius_view);

	QRect viewport_rect = m_view->viewport()->rect();

	if (view_pos.x() + overlay_width > viewport_rect.right()) {
		view_pos.setX(star_view.x() - annulus_radius_view - overlay_width);
	}
	if (view_pos.y() + overlay_height > viewport_rect.bottom()) {
		view_pos.setY(star_view.y() - annulus_radius_view - overlay_height);
	}

	m_snr_overlay->move(view_pos);
}

void ImageViewer::mouseRightPressAt(double x, double y, Qt::KeyboardModifiers modifiers) {
	indigo_debug("RIGHT CLICK COORDS: %f %f", x, y);

	double ra, dec, telescope_ra, telescope_dec;
	if (m_pixmap->image().valid(x, y)) {
		moveSelection(x, y);
		emit mouseRightPress(x, y, modifiers);
		if (
			m_pixmap->image().wcs_data(x, y, &ra, &dec, &telescope_ra, &telescope_dec) == 0 &&
			m_show_wcs
		) {
			emit mouseRightPressRADec(ra, dec, telescope_ra, telescope_dec, modifiers);
		}
	}
}

// Handle Ctrl+Left-click for SNR calculation
void ImageViewer::mouseLeftPressAt(double x, double y, Qt::KeyboardModifiers modifiers) {
	if ((modifiers & Qt::ControlModifier) && m_snr_mode_enabled) {
		calculateAndShowSNR(x, y);
		return;
	}

	// Hide SNR overlay if clicking elsewhere without Ctrl
	if (m_snr_overlay_visible) {
		showSNROverlay(false);
	}
}

void ImageViewer::mouseLeftDoubleClickAt(double x, double y, Qt::KeyboardModifiers modifiers) {
	if (m_snr_mode_enabled) {
		calculateAndShowSNR(x, y);
	}
}

void ImageViewer::enableSNRMode(bool enable) {
	m_snr_mode_enabled = enable;
	if (!enable && m_snr_overlay_visible) {
		showSNROverlay(false);
	}
}

void ImageViewer::enterEvent(QEvent *event) {
	QFrame::enterEvent(event);
	if (m_bar_mode == ToolBarMode::AutoHidden) {
		m_toolbar->show();
		if (m_fit)
			zoomFit();
	}
}

void ImageViewer::leaveEvent(QEvent *event) {
	QFrame::leaveEvent(event);
	if (m_bar_mode == ToolBarMode::AutoHidden) {
		m_toolbar->hide();
		if (m_fit)
			zoomFit();
	}
	emit mouseAt(-1, -1);
}

void ImageViewer::resizeEvent(QResizeEvent *event) {
	QFrame::resizeEvent(event);
	if (m_fit)
		zoomFit();
	emit viewerResized();
}

void ImageViewer::showEvent(QShowEvent *event) {
	QFrame::showEvent(event);
	if (m_fit)
		zoomFit();
	emit viewerShown();
}

void ImageViewer::stretchNone() {
	emit stretchChanged(PREVIEW_STRETCH_NONE);
}

void ImageViewer::stretchSlight() {
	emit stretchChanged(PREVIEW_STRETCH_SLIGHT);
}

void ImageViewer::stretchModerate() {
	emit stretchChanged(PREVIEW_STRETCH_MODERATE);
}

void ImageViewer::stretchNormal() {
	emit stretchChanged(PREVIEW_STRETCH_NORMAL);
}

void ImageViewer::stretchHard() {
	emit stretchChanged(PREVIEW_STRETCH_HARD);
}

void ImageViewer::debayerAuto() {
	emit debayerChanged(BAYER_PAT_AUTO);
}

void ImageViewer::debayerNone() {
	emit debayerChanged(BAYER_PAT_NONE);
}

void ImageViewer::debayerGBRG() {
	emit debayerChanged(BAYER_PAT_GBRG);
}

void ImageViewer::debayerGRBG() {
	emit debayerChanged(BAYER_PAT_GRBG);
}

void ImageViewer::debayerRGGB() {
	emit debayerChanged(BAYER_PAT_RGGB);
}

void ImageViewer::debayerBGGR() {
	emit debayerChanged(BAYER_PAT_BGGR);
}

void ImageViewer::onAutoBalance() {
	emit BalanceChanged(COLOR_BALANCE_AUTO);
}

void ImageViewer::onNoBalance() {
	emit BalanceChanged(COLOR_BALANCE_NONE);
}

void ImageViewer::onPrevious() {
	emit previousRequested();
}

void ImageViewer::onNext() {
	emit nextRequested();
}

void ImageViewer::setStretch(int level) {
	switch (level) {
		case PREVIEW_STRETCH_NONE:
			m_stretch_act[PREVIEW_STRETCH_NONE]->setChecked(true);
			stretchNone();
			break;
		case PREVIEW_STRETCH_SLIGHT:
			m_stretch_act[PREVIEW_STRETCH_SLIGHT]->setChecked(true);
			stretchSlight();
			break;
		case PREVIEW_STRETCH_MODERATE:
			m_stretch_act[PREVIEW_STRETCH_MODERATE]->setChecked(true);
			stretchModerate();
			break;
		case PREVIEW_STRETCH_NORMAL:
			m_stretch_act[PREVIEW_STRETCH_NORMAL]->setChecked(true);
			stretchNormal();
			break;
		case PREVIEW_STRETCH_HARD:
			m_stretch_act[PREVIEW_STRETCH_HARD]->setChecked(true);
			stretchHard();
			break;
		default:
			m_stretch_act[PREVIEW_STRETCH_NONE]->setChecked(true);
			stretchNone();
	}
}

void ImageViewer::setDebayer(uint32_t bayer_pat) {
	switch (bayer_pat) {
		case 0:
		case BAYER_PAT_AUTO:
			m_debayer_act[DEBAYER_AUTO]->setChecked(true);
			debayerAuto();
			break;
		case BAYER_PAT_NONE:
			m_debayer_act[DEBAYER_NONE]->setChecked(true);
			debayerNone();
			break;
		case BAYER_PAT_GBRG:
			m_debayer_act[DEBAYER_GBRG]->setChecked(true);
			debayerGBRG();
			break;
		case BAYER_PAT_GRBG:
			m_debayer_act[DEBAYER_GRBG]->setChecked(true);
			debayerGRBG();
			break;
		case BAYER_PAT_RGGB:
			m_debayer_act[DEBAYER_RGGB]->setChecked(true);
			debayerRGGB();
			break;
		case BAYER_PAT_BGGR:
			m_debayer_act[DEBAYER_BGGR]->setChecked(true);
			debayerBGGR();
			break;
		default:
			m_debayer_act[DEBAYER_AUTO]->setChecked(true);
			debayerAuto();
	}
}

void ImageViewer::setBalance(int balance) {
	switch (balance) {
		case COLOR_BALANCE_AUTO:
			m_color_reference_act[COLOR_BALANCE_AUTO]->setChecked(true);
			onAutoBalance();
			break;
		case COLOR_BALANCE_NONE:
			m_color_reference_act[COLOR_BALANCE_NONE]->setChecked(true);
			onNoBalance();
			break;
		default:
			m_color_reference_act[COLOR_BALANCE_AUTO]->setChecked(true);
			onAutoBalance();
	}
}

QRect ImageViewer::getImageFrameRect() const {
	QRect frameRect = this->rect();

	if (m_bar_mode == ToolBarMode::AutoHidden || m_bar_mode == ToolBarMode::Visible) {
		QRect toolbarRect = m_toolbar->rect();
		frameRect = QRect(
			frameRect.x(),
			frameRect.y() + toolbarRect.height(),
			frameRect.width(),
			frameRect.height() - toolbarRect.height()
		);
	}

	return frameRect;
}

PixmapItem::PixmapItem(QGraphicsItem *parent) :
	QObject(), QGraphicsPixmapItem(parent), m_is_double_click(false)
{
	//setTransformationMode(Qt::SmoothTransformation);
	setAcceptHoverEvents(true);
}

void PixmapItem::setImage(preview_image im) {
	//if (im.isNull()) return;

	auto image_size = m_image.size();
	m_image = im;
	indigo_debug("%s MIMAGE m_raw_data = %p",__FUNCTION__, m_image.m_raw_data);

	setPixmap(QPixmap::fromImage(m_image));

	if (image_size != m_image.size())
		emit sizeChanged(m_image.width(), m_image.height());

	emit imageChanged(m_image);
}

void PixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if(event->button() == Qt::RightButton) {
		auto pos = event->pos();
		emit mouseRightPress(pos.x(), pos.y(), event->modifiers());
	} else if(event->button() == Qt::LeftButton) {
		// Don't emit single-click signal if this is part of a double-click
		if (!m_is_double_click) {
			auto pos = event->pos();
			emit mouseLeftPress(pos.x(), pos.y(), event->modifiers());
		}
		m_is_double_click = false;
	}
	QGraphicsItem::mousePressEvent(event);
}

void PixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	QGraphicsItem::mouseReleaseEvent(event);
}

void PixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if(event->button() == Qt::LeftButton) {
		m_is_double_click = true;
		auto pos = event->pos();
		emit mouseLeftDoubleClick(pos.x(), pos.y(), event->modifiers());
	}
	QGraphicsItem::mouseDoubleClickEvent(event);
}

void PixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
	auto pos = event->pos();
	emit mouseMoved(pos.x(), pos.y());
	QGraphicsItem::hoverMoveEvent(event);
}
