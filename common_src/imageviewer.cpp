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


ImageViewer::ImageViewer(QWidget *parent, bool prev_next)
	: QFrame(parent)
	, m_zoom_level(0)
	, m_fit(true)
	, m_bar_mode(ToolBarMode::Visible)
{
	auto scene = new QGraphicsScene(this);
	m_view = new GraphicsView(this);
	m_view->setScene(scene);

	// graphic object holding the image buffer
	m_pixmap = new PixmapItem;
	scene->addItem(m_pixmap);
	connect(m_pixmap, SIGNAL(mouseMoved(double,double)), SLOT(mouseAt(double,double)));
	connect(m_pixmap, SIGNAL(mouseRightPress(double, double, Qt::KeyboardModifiers)), SLOT(mouseRightPressAt(double, double, Qt::KeyboardModifiers)));

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

	makeToolbar(prev_next);

	auto box = new QVBoxLayout;
	box->setContentsMargins(0,0,0,0);
	box->addWidget(m_toolbar);
	box->addWidget(m_view, 1);
	setLayout(box);

	m_image_stats = new QLabel(m_view);
	m_image_stats->setStyleSheet("background-color: rgba(0,0,0,35%); color: rgba(200,200,200,100%);");
	m_image_stats->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	m_image_stats->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_image_stats->move(QPoint(15, 15));
	m_image_stats->setTextFormat(Qt::RichText);
	m_image_stats->raise();
	m_image_stats->setVisible(false);

	m_extra_selections_visible = false;

	connect(this, &ImageViewer::setImage, this, &ImageViewer::onSetImage);
}

// toolbar with a few quick actions and display information
void ImageViewer::makeToolbar(bool prev_next) {
	// text and value at pixel
	m_text_label = new QLabel(this);
	m_text_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	m_pixel_value = new QLabel(this);

	auto fit = new QToolButton(this);
	fit->setToolTip(tr("Fit image to window"));
	fit->setIcon(QIcon(":resource/zoom-fit-best.png"));
	connect(fit, SIGNAL(clicked()), SLOT(zoomFit()));

	auto orig = new QToolButton(this);
	orig->setToolTip(tr("Zoom 1:1"));
	orig->setIcon(QIcon(":resource/zoom-original.png"));
	connect(orig, SIGNAL(clicked()), SLOT(zoomOriginal()));

	auto zoomin = new QToolButton(this);
	zoomin->setToolTip(tr("Zoom In"));
	zoomin->setIcon(QIcon(":resource/zoom-in.png"));
	connect(zoomin, SIGNAL(clicked()), SLOT(zoomIn()));

	auto zoomout = new QToolButton(this);
	zoomout->setToolTip(tr("Zoom Out"));
	zoomout->setIcon(QIcon(":resource/zoom-out.png"));
	connect(zoomout, SIGNAL(clicked()), SLOT(zoomOut()));

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

	auto stretch = new QToolButton(this);
	stretch->setToolTip(tr("Histogram Stretch"));
	stretch->setIcon(QIcon(":resource/histogram.png"));
	stretch->setMenu(menu);
	stretch->setPopupMode(QToolButton::InstantPopup);

	m_toolbar = new QWidget;
	auto box = new QHBoxLayout(m_toolbar);
	m_toolbar->setContentsMargins(0,0,0,0);
	box->setContentsMargins(0,0,0,0);
	if (prev_next) {
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

	box->addWidget(zoomout);
	box->addWidget(zoomin);
	box->addWidget(fit);
	box->addWidget(orig);
	box->addWidget(stretch);
}

void ImageViewer::setImageStats(const ImageStats &stats) {
	if (stats.channels == 1) {
		QString stats_str = "<p><b>Statistics</b></p>";
		stats_str += "<p><b><font color=\"#808080\">Mono</font></b><br>";
		stats_str += "<b>Min:</b>  " + QString::number(stats.grey_red.min) + "<br>";
		stats_str += "<b>Max:</b>  " + QString::number(stats.grey_red.max) + "<br>";
		stats_str += "<b>Mean:</b>  " + QString::number(stats.grey_red.mean) + "<br>";
		stats_str += "<b>StdDev:</b>  " + QString::number(stats.grey_red.stddev) + "<br>";
		stats_str += "<b>MAD:</b>  " + QString::number(stats.grey_red.mad) + "</p>";
		m_image_stats->setText(stats_str);
		m_image_stats->adjustSize();
		m_image_stats->setVisible(true);
	} else if (stats.channels == 3) {
		QString stats_str = "<p><b>Statistics</b></p>";
		stats_str += "<p><b><font color=\"#A00000\">Red</font></b><br>";
		stats_str += "<b>Min:</b>  " + QString::number(stats.grey_red.min) + "<br>";
		stats_str += "<b>Max:</b>  " + QString::number(stats.grey_red.max) + "<br>";
		stats_str += "<b>Mean:</b>  " + QString::number(stats.grey_red.mean) + "<br>";
		stats_str += "<b>StdDev:</b>  " + QString::number(stats.grey_red.stddev) + "<br>";
		stats_str += "<b>MAD:</b>  " + QString::number(stats.grey_red.mad) + "</p>";

		stats_str += "<p><b><font color=\"#00A000\">Green</font></b><br>";
		stats_str += "<b>Min:</b>  " + QString::number(stats.green.min) + "<br>";
		stats_str += "<b>Max:</b>  " + QString::number(stats.green.max) + "<br>";
		stats_str += "<b>Mean:</b>  " + QString::number(stats.green.mean) + "<br>";
		stats_str += "<b>StdDev:</b>  " + QString::number(stats.green.stddev) + "<br>";
		stats_str += "<b>MAD:</b>  " + QString::number(stats.green.mad) + "</p>";

		stats_str += "<p><b><font color=\"#3030F0\">Blue</font></b><br>";
		stats_str += "<b>Min:</b>  " + QString::number(stats.blue.min) + "<br>";
		stats_str += "<b>Max:</b>  " + QString::number(stats.blue.max) + "<br>";
		stats_str += "<b>Mean:</b>  " + QString::number(stats.blue.mean) + "<br>";
		stats_str += "<b>StdDev:</b>  " + QString::number(stats.blue.stddev) + "<br>";
		stats_str += "<b>MAD:</b>  " + QString::number(stats.blue.mad) + "</p>";

		m_image_stats->setText(stats_str);
		m_image_stats->adjustSize();
		m_image_stats->setVisible(true);
	} else {
		m_image_stats->setVisible(false);
		m_image_stats->setText("");
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
		s.sprintf("%.0f%%", m_zoom_level);
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

	QMatrix matrix;
	matrix.scale(scale, scale);

	m_view->setMatrix(matrix);
	emit zoomChanged(m_view->matrix().m11());
}

void ImageViewer::zoomFit() {
	m_view->fitInView(m_pixmap, Qt::KeepAspectRatio);
	m_zoom_level = (100.0 * m_view->matrix().m11());
	showZoom();
	indigo_debug("Zoom FIT = %.2f", m_zoom_level);
	m_fit = true;
	emit zoomChanged(m_view->matrix().m11());
}

void ImageViewer::zoomOriginal() {
	m_zoom_level = 100;
	showZoom();
	indigo_debug("Zoom 1:1 = %.2f", m_zoom_level);
	m_fit = false;
	setMatrix();
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
}

void ImageViewer::mouseAt(double x, double y) {
	if (m_pixmap && m_pixmap->image().valid(x,y)) {
		double r,g,b;
		double ra, dec;
		int pix_format = m_pixmap->image().pixel_value(x, y, r, g, b);
		int res = m_pixmap->image().wcs_data(x, y, &ra, &dec);
		QString s;
		if (res != -1 && m_show_wcs) {
			s.sprintf("%.0f%% [%5.1f, %5.1f] (%s, %s) ", m_zoom_level, x, y, indigo_dtos(ra / 15, "%dh %02d' %04.1f\""), indigo_dtos(dec, "%+d° %02d' %04.1f\""));
		} else {
			if (pix_format == PIX_FMT_INDEX) {
				s.sprintf("%.0f%% [%5.1f, %5.1f]", m_zoom_level, x, y);
			} else if (pix_format == PIX_FMT_F32 || pix_format == PIX_FMT_RGBF){
				if (g == -1) {
					s.sprintf("%.0f%% [%5.1f, %5.1f] (%.6f)", m_zoom_level, x, y, r);
				} else {
					s.sprintf("%.0f%% [%5.1f, %5.1f] (%.6f, %.6f, %.6f)", m_zoom_level, x, y, r, g, b);
				}
			} else {
				if (g == -1) {
					s.sprintf("%.0f%% [%5.1f, %5.1f] (%5.0f)", m_zoom_level, x, y, r);
				} else {
					s.sprintf("%.0f%% [%5.1f, %5.1f] (%5.0f, %5.0f, %5.0f)", m_zoom_level, x, y, r, g, b);
				}
			}
		}
		m_pixel_value->setText(s);
	} else {
		showZoom();
	}
}

void ImageViewer::mouseRightPressAt(double x, double y, Qt::KeyboardModifiers modifiers) {
	indigo_debug("RIGHT CLICK COORDS: %f %f" ,x,y);
	double ra, dec, telescope_ra, telescope_dec;
	if (m_pixmap->image().valid(x,y)) {
		moveSelection(x,y);
		emit mouseRightPress(x, y, modifiers);
		if (
			m_pixmap->image().wcs_data(x, y, &ra, &dec, &telescope_ra, &telescope_dec) == 0 &&
			m_show_wcs
		) {
			emit mouseRightPressRADec(ra, dec, telescope_ra, telescope_dec, modifiers);
		}
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
}

void ImageViewer::showEvent(QShowEvent *event) {
	QFrame::showEvent(event);
	if (m_fit)
		zoomFit();
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

PixmapItem::PixmapItem(QGraphicsItem *parent) :
	QObject(), QGraphicsPixmapItem(parent)
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
		}
	QGraphicsItem::mousePressEvent(event);
}

void PixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	QGraphicsItem::mouseReleaseEvent(event);
}

void PixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
	auto pos = event->pos();
	emit mouseMoved(pos.x(), pos.y());
	QGraphicsItem::hoverMoveEvent(event);
}
