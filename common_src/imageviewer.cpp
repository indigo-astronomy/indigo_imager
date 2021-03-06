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
	, m_stretch_level(PREVIEW_STRETCH_NONE)
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
	connect(m_pixmap, SIGNAL(mouseRightPress(double,double)), SLOT(mouseRightPressAt(double,double)));

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

	m_extra_selections_visible = false;
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

QString ImageViewer::text() const {
	return m_text_label->text();
}

void ImageViewer::setText(const QString &txt) {
	m_text_label->setText(txt);
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

void ImageViewer::setImage(preview_image &im) {
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
	m_zoom_level = 100.0 * m_view->matrix().m11();
	indigo_debug("Zoom FIT = %d", m_zoom_level);
	m_fit = true;
	emit zoomChanged(m_view->matrix().m11());
}

void ImageViewer::zoomOriginal() {
	m_zoom_level = 100;
	indigo_debug("Zoom 1:1 = %d", m_zoom_level);
	m_fit = false;
	setMatrix();
}

void ImageViewer::zoomIn() {
	if (m_zoom_level >= 1000) {
		m_zoom_level = (int)(m_zoom_level / 500) * 500 + 500;
	} else if (m_zoom_level >= 100) {
		m_zoom_level = (int)(m_zoom_level / 50) * 50 + 50;
	} else if (m_zoom_level >= 10) {
		m_zoom_level = (int)(m_zoom_level / 10) * 10 + 10;
	} else if (m_zoom_level >= 1) {
		m_zoom_level += 1;
	} else {
		m_zoom_level = 1;
	}
	if (m_zoom_level > 5000) {
		m_zoom_level = 5000;
	}
	indigo_debug("Zoom IN = %d", m_zoom_level);
	m_fit = false;
	setMatrix();
}

void ImageViewer::zoomOut() {
	if (m_zoom_level > 1000) {
		m_zoom_level = (int)(round(m_zoom_level / 500.0)) * 500 - 500;
	} else if (m_zoom_level > 100) {
		m_zoom_level = (int)(round(m_zoom_level / 50.0)) * 50 - 50;
	} else if (m_zoom_level > 10) {
		m_zoom_level = (int)(round(m_zoom_level / 10.0)) * 10 - 10;
	} else if (m_zoom_level > 1) {
		m_zoom_level -= 1;
	} else {
		m_zoom_level = 1;
	}
	indigo_debug("Zoom OUT = %d", m_zoom_level);
	m_fit = false;
	setMatrix();
}

void ImageViewer::mouseAt(double x, double y) {
	if (m_pixmap->image().valid(x,y)) {
		int r,g,b;
		int pix_format = m_pixmap->image().pixel_value(x, y, r, g, b);

		QString s;
		if (pix_format == PIX_FMT_INDEX) {
			s.sprintf("%d%% [%5.1f, %5.1f]", m_zoom_level, x, y);
		} else {
			if (g == -1) {
				//s = QString("%1% [%2, %3] (%4)").arg(scale).arg(x).arg(y).arg(r);
				s.sprintf("%d%% [%5.1f, %5.1f] (%5d)", m_zoom_level, x, y, r);
			} else {
				//s = QString("%1% [%2, %3] (%4, %5, %6)").arg(scale).arg(x).arg(y).arg(r).arg(g).arg(b);
				s.sprintf("%d%% [%5.1f, %5.1f] (%5d, %5d, %5d)", m_zoom_level, x, y, r, g, b);
			}
		}
		m_pixel_value->setText(s);
	} else {
		m_pixel_value->setText(QString());
	}
}

void ImageViewer::mouseRightPressAt(double x, double y) {
	indigo_debug("RIGHT CLICK COORDS: %f %f" ,x,y);
	if (m_pixmap->image().valid(x,y)) {
		moveSelection(x,y);
		emit mouseRightPress(x,y);
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
	m_stretch_level = PREVIEW_STRETCH_NONE;
	preview_image &image = (preview_image&)m_pixmap->image();
	stretch_preview(&image, preview_stretch_lut[m_stretch_level]);
	setImage(image);
	emit stretchChanged(m_stretch_level);
}

void ImageViewer::stretchSlight() {
	m_stretch_level = PREVIEW_STRETCH_SLIGHT;
	preview_image &image = (preview_image&)m_pixmap->image();
	stretch_preview(&image, preview_stretch_lut[m_stretch_level]);
	setImage(image);
	emit stretchChanged(m_stretch_level);
}

void ImageViewer::stretchModerate() {
	m_stretch_level = PREVIEW_STRETCH_MODERATE;
	preview_image &image = (preview_image&)m_pixmap->image();
	stretch_preview(&image, preview_stretch_lut[m_stretch_level]);
	setImage(image);
	emit stretchChanged(m_stretch_level);
}

void ImageViewer::stretchNormal() {
	m_stretch_level = PREVIEW_STRETCH_NORMAL;
	preview_image &image = (preview_image&)m_pixmap->image();
	stretch_preview(&image, preview_stretch_lut[m_stretch_level]);
	setImage(image);
	emit stretchChanged(m_stretch_level);
}

void ImageViewer::stretchHard() {
	m_stretch_level = PREVIEW_STRETCH_HARD;
	preview_image &image = (preview_image&)m_pixmap->image();
	stretch_preview(&image, preview_stretch_lut[m_stretch_level]);
	setImage(image);
	emit stretchChanged(m_stretch_level);
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
			emit mouseRightPress(pos.x(), pos.y());
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
