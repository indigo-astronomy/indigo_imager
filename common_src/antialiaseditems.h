#ifndef ANTIALIASED_ITEMS_H
#define ANTIALIASED_ITEMS_H

#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
QT_END_NAMESPACE

class AntialiasedEllipseItem : public QGraphicsEllipseItem {
public:
    using QGraphicsEllipseItem::QGraphicsEllipseItem;
    virtual ~AntialiasedEllipseItem();

    static void paintEllipse(QPainter *painter, const QRectF &rect, const QPen &pen, const QBrush &brush,
        const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

class AntialiasedRectItem : public QGraphicsRectItem {
public:
    using QGraphicsRectItem::QGraphicsRectItem;
    virtual ~AntialiasedRectItem();

    static void paintRect(QPainter *painter, const QRectF &rect, const QPen &pen, const QBrush &brush,
        const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // ANTIALIASED_ITEMS_H
