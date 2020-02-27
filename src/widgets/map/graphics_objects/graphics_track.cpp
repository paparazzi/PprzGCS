#include "graphics_track.h"
#include <QPainter>

GraphicsTrack::GraphicsTrack(QColor color_idle, QColor color_unfocused, QObject *parent) :
    GraphicsObject(parent), color_idle(color_idle), color_unfocused(color_unfocused)
{
    current_color = &color_idle;
}

GraphicsTrack::GraphicsTrack(QColor color_idle, QColor color_unfocused, QPolygonF points, QObject *parent) :
    GraphicsTrack(color_idle, color_unfocused, parent)
{
    this->points = points;
}

void GraphicsTrack::addPoint(QPointF pt) {
    points.append(pt);
    prepareGeometryChange();
    //update();
}

void GraphicsTrack::setPoints(QPolygonF pts) {
    points = pts;
    prepareGeometryChange();
    //update();
}

QRectF GraphicsTrack::boundingRect() const {
    return points.boundingRect();
}

void GraphicsTrack::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;
    QPen pen = painter->pen();
    pen.setColor(*current_color);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawPolyline(points);
}

void GraphicsTrack::changeFocus() {
    if(!isHighlighted()) {
        current_color = &color_unfocused;
    } else {
        current_color = &color_idle;
    }
    update();
}
