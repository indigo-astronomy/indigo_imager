#include "image-viewer.h"
#include <cmath>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneHoverEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>

namespace pal {

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
                m_viewer->zoomIn(3);
            else if (event->delta() < 0)
                m_viewer->zoomOut(3);
            event->accept();
        }
        else
            QGraphicsView::wheelEvent(event);
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


ImageViewer::ImageViewer(QWidget *parent)
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
    connect(m_pixmap, SIGNAL(mouseMoved(int,int)), SLOT(mouseAt(int,int)));
	connect(m_pixmap, SIGNAL(mouseRightPress(int,int)), SLOT(mouseRightPressAt(int,int)));

	m_ref_x = new QGraphicsLineItem(25,0,25,50, m_pixmap);
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(Qt::yellow);
	m_ref_x->setPen(pen);
	m_ref_x->setOpacity(0.4);
	m_ref_x->setVisible(false);
	scene->addItem(m_ref_x);

	m_ref_y = new QGraphicsLineItem(0,25,50,25, m_pixmap);
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(Qt::yellow);
	m_ref_y->setPen(pen);
	m_ref_y->setOpacity(0.4);
	m_ref_y->setVisible(false);
	scene->addItem(m_ref_y);

	m_ref_visible = false;

	m_selection = new QGraphicsRectItem(0,0,25,25, m_pixmap);
	m_selection->setBrush(QBrush(Qt::NoBrush));
	pen.setCosmetic(true);
	pen.setWidth(1);
	pen.setColor(Qt::green);
	m_selection->setPen(pen);
	m_selection->setOpacity(0.6);
	m_selection->setVisible(false);
	m_selection_visible = false;
	scene->addItem(m_selection);
	//m_selection->setFlags(QGraphicsItem::ItemIsMovable);

    makeToolbar();

    auto box = new QVBoxLayout;
    box->setContentsMargins(0,0,0,0);
    box->addWidget(m_toolbar);
    box->addWidget(m_view, 1);
    setLayout(box);
}

// toolbar with a few quick actions and display information
void ImageViewer::makeToolbar() {
    // text and value at pixel
    m_text_label = new QLabel(this);
    m_text_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
    m_pixel_value = new QLabel(this);

    auto fit = new QToolButton(this);
    fit->setToolTip(tr("Fit image to window"));
    fit->setIcon(QIcon(":resource/zoom-fit-best.png"));
    connect(fit, SIGNAL(clicked()), SLOT(zoomFit()));

    auto orig = new QToolButton(this);
    orig->setToolTip(tr("Resize image to its original size"));
    orig->setIcon(QIcon(":resource/zoom-original.png"));
    connect(orig, SIGNAL(clicked()), SLOT(zoomOriginal()));

    m_toolbar = new QWidget;
    auto box = new QHBoxLayout(m_toolbar);
    m_toolbar->setContentsMargins(0,0,0,0);
    box->setContentsMargins(0,0,0,0);
    box->addWidget(m_text_label);
    box->addStretch(1);
    box->addWidget(m_pixel_value);
    box->addWidget(fit);
    box->addWidget(orig);
}

QString ImageViewer::text() const {
    return m_text_label->text();
}

void ImageViewer::setText(const QString &txt) {
    m_text_label->setText(txt);
}

void ImageViewer::showSelection() {
	m_selection_visible = true;
	if (!m_pixmap->pixmap().isNull() && !m_selection_p.isNull()) {
		m_selection->setVisible(true);
	}
}

void ImageViewer::hideSelection() {
	m_selection_visible = false;
	m_selection->setVisible(false);
}

void ImageViewer::moveResizeSelection(double x, double y, int size) {
	double cor_x = x - (size) / 2.0;
	double cor_y = y - (size) / 2.0;

	if (!m_pixmap->pixmap().isNull() && ((cor_x < 0) || (cor_y < 0) ||
	    (cor_x > m_pixmap->pixmap().width() - size + 1) ||
	    (cor_y > m_pixmap->pixmap().height() - size + 1))) {
		return;
	}
	indigo_debug("%.2f -> %.2f, %.2f -> %.2f, %d", x, cor_x, y, cor_y, size);
	m_selection_p.setX(x);
	m_selection_p.setY(y);
	if (m_selection_p.isNull()) {
		m_selection->setVisible(false);
	} else if (m_selection_visible){
		m_selection->setVisible(true);
	}
	m_selection->setRect(0, 0, size, size);
	m_selection->setPos(cor_x, cor_y);
}

void ImageViewer::moveSelection(double x, double y) {
	QRectF br = m_selection->boundingRect();
	double cor_x = x - br.width() / 2.0;
	double cor_y = y - br.height() / 2.0;

	if (!m_pixmap->pixmap().isNull() && ((cor_x < 0) || (cor_y < 0) ||
	    (cor_x > m_pixmap->pixmap().width() - (int)br.width() + 1) ||
	    (cor_y > m_pixmap->pixmap().height() - (int)br.height() + 1))) {
		return;
	}
	indigo_debug("%.2f -> %.2f, %.2f -> %.2f, %d", x, cor_x, y, cor_y, (int)br.width());
	m_selection_p.setX(x);
	m_selection_p.setY(y);
	m_selection->setPos(cor_x, cor_y);
}

void ImageViewer::showReference() {
	m_ref_visible = true;
	if (!m_pixmap->pixmap().isNull() && !m_ref_p.isNull()) {
		m_ref_x->setVisible(true);
		m_ref_y->setVisible(true);
	}
}

void ImageViewer::hideReference() {
	m_ref_visible = false;
	m_ref_x->setVisible(false);
	m_ref_y->setVisible(false);
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
    qreal scale = std::pow(2.0, m_zoom_level / 10.0);

    QMatrix matrix;
    matrix.scale(scale, scale);

    m_view->setMatrix(matrix);
    emit zoomChanged(m_view->matrix().m11());
}

void ImageViewer::zoomFit() {
    m_view->fitInView(m_pixmap, Qt::KeepAspectRatio);
    m_zoom_level = int(10.0 * std::log2(m_view->matrix().m11()));
    m_fit = true;
    emit zoomChanged(m_view->matrix().m11());
}

void ImageViewer::zoomOriginal() {
    m_zoom_level = 0;
    m_fit = false;
    setMatrix();
}

void ImageViewer::zoomIn(int level) {
    m_zoom_level += level;
    m_fit = false;
    setMatrix();
}

void ImageViewer::zoomOut(int level) {
    m_zoom_level -= level;
    m_fit = false;
	setMatrix();
}

void ImageViewer::mouseAt(int x, int y) {
	//indigo_log("COORDS: %d %d" ,x,y);
	if (m_pixmap->image().valid(x,y)) {
		int r,g,b;
		qreal scale = std::pow(2.0, m_zoom_level / 10.0) * 100;
		m_pixmap->image().pixel_value(x, y, r, g, b);
		QString s;
		if (g == -1) {
			//s = QString("%1% [%2, %3] (%4)").arg(scale).arg(x).arg(y).arg(r);
			s.sprintf("%.2f%% [%5d, %5d] (%5d)", scale, x, y, r);
		} else {
			//s = QString("%1% [%2, %3] (%4, %5, %6)").arg(scale).arg(x).arg(y).arg(r).arg(g).arg(b);
			s.sprintf("%.2f%% [%5d, %5d] (%5d, %5d, %5d)", scale, x, y, r, g, b);
		}
		m_pixel_value->setText(s);
	} else {
		m_pixel_value->setText(QString());
	}
}

void ImageViewer::mouseRightPressAt(int x, int y) {
	indigo_log("RIGHT CLICK COORDS: %d %d" ,x,y);
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


PixmapItem::PixmapItem(QGraphicsItem *parent) :
    QObject(), QGraphicsPixmapItem(parent)
{
	//setTransformationMode(Qt::SmoothTransformation);
    setAcceptHoverEvents(true);
}

void PixmapItem::setImage(preview_image im) {
	if (im.isNull()) return;

	auto image_size = m_image.size();
	m_image = im;
	indigo_error("%s MIMAGE m_raw_data = %p",__FUNCTION__, m_image.m_raw_data);

	setPixmap(QPixmap::fromImage(m_image));

	if (image_size != m_image.size())
		emit sizeChanged(m_image.width(), m_image.height());

	emit imageChanged(m_image);
}

void PixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
		if(event->button() == Qt::RightButton) {
			auto pos = event->pos();
			emit mouseRightPress(int(pos.x()), int(pos.y()));
		}
	QGraphicsItem::mousePressEvent(event);
}

void PixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsItem::mouseReleaseEvent(event);
}

void PixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    auto pos = event->pos();
    emit mouseMoved(int(pos.x()), int(pos.y()));
    QGraphicsItem::hoverMoveEvent(event);
}

} // namespace pal
