#include "graphics_line.h"
#include <QDebug>


GraphicsLine::GraphicsLine(QLineF linef, QPen pen_idle, QObject *parent) :
    GraphicsObject(parent),
    QGraphicsLineItem (linef),
    pen_idle(pen_idle), pen_unfocused(pen_idle), ignore_events(false)
{
    setPen(pen_idle);
}


QRectF GraphicsLine::boundingRect() const {
    QRectF rect = QGraphicsLineItem::boundingRect();
    qDebug() << rect;
    return rect;
}


void GraphicsLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QGraphicsLineItem::paint(painter, option, widget);
}


void GraphicsLine::changeFocus() {
    if(!isHighlighted()) {
        setPen(pen_unfocused);
    } else {
        setPen(pen_idle);
    }
}


void GraphicsLine::setColors(QColor color) {
    pen_unfocused = pen();
    pen_unfocused.setColor(color);
}

void GraphicsLine::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    GraphicsObject::mousePressEvent(event);
    qDebug() << "Line PRESSED !";
}

void GraphicsLine::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    (void)event;
}

void GraphicsLine::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    (void)event;
}
