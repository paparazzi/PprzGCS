#include "graphicsline.h"
#include <QDebug>


GraphicsLine::GraphicsLine(QLineF linef, QPen pen_idle, QObject *parent) :
    GraphicsObject(parent),
    QGraphicsLineItem (linef),
    pen_idle(pen_idle), pen_unfocused(pen_idle)
{
    setPen(pen_idle);
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
    GraphicsObject::mousePressEvent(event);
    qDebug() << "Line PRESSED !";
}

void GraphicsLine::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    (void)event;
}

void GraphicsLine::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    (void)event;
}
