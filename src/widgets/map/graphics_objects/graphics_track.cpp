#include "graphics_track.h"
#include <QPainter>

#define COLOR_IDLE 0
#define COLOR_UNFOCUSED 3

GraphicsTrack::GraphicsTrack(PprzPalette palette, QObject *parent) :
    GraphicsObject(palette, parent), current_color(COLOR_IDLE)
{

}

GraphicsTrack::GraphicsTrack(PprzPalette palette, QPolygonF points, QObject *parent) :
    GraphicsTrack(palette, parent)
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
    pen.setColor(palette.getVariant(current_color));
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawPolyline(points);
}

void GraphicsTrack::changeFocus() {
    if(!isHighlighted()) {
        current_color = COLOR_UNFOCUSED;
    } else {
        current_color = COLOR_IDLE;
    }
    update();
}
