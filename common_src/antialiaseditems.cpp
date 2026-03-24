#include "antialiaseditems.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

void AntialiasedEllipseItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QGraphicsEllipseItem::paint(painter, option, widget);
    painter->restore();
}

void AntialiasedEllipseItem::paintEllipse(QPainter *painter, const QRectF &rect, const QPen &pen, const QBrush &brush,
    const QStyleOptionGraphicsItem *option, QWidget *widget) {
    AntialiasedEllipseItem item(rect);
    item.setPen(pen);
    item.setBrush(brush);
    item.paint(painter, option, widget);
}

AntialiasedEllipseItem::~AntialiasedEllipseItem() {
    // out-of-line destructor to ensure vtable is emitted
}

void AntialiasedRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QGraphicsRectItem::paint(painter, option, widget);
    painter->restore();
}

void AntialiasedRectItem::paintRect(QPainter *painter, const QRectF &rect, const QPen &pen, const QBrush &brush,
    const QStyleOptionGraphicsItem *option, QWidget *widget) {
    AntialiasedRectItem item(rect);
    item.setPen(pen);
    item.setBrush(brush);
    item.paint(painter, option, widget);
}

AntialiasedRectItem::~AntialiasedRectItem() {
    // out-of-line destructor to ensure vtable is emitted
}
