#include "mapitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>

MapItem::MapItem(Point2DLatLon pt, QGraphicsItem *parent) : QGraphicsItemGroup(parent),
    move_state(MIS_IDLE), movable(true), latlon(pt), zoom_factor(1), neutral_scale_zoom(15)
{
}

MapItem::MapItem(QGraphicsItem* child, Point2DLatLon pt, double neutral_scale_zoom, QGraphicsItem *parent) : QGraphicsItemGroup(parent),
    move_state(MIS_IDLE), movable(true), latlon(pt), zoom_factor(1), neutral_scale_zoom(neutral_scale_zoom)
{
    addToGroup(child);
}

void MapItem::scaleToZoom(double zoom, double viewScale) {
    double s = pow(zoom_factor, zoom - neutral_scale_zoom)/viewScale;
    setScale(s);
}

void MapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    move_state = MIS_PRESSED;
}

void MapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(move_state == MIS_PRESSED) {
        QPointF dp = event->pos() - pressPos;
        double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
        if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
            move_state = MIS_MOVED;
        }
    } else if(move_state == MIS_MOVED) {
        if(movable) {
            setPos(event->scenePos() - pressPos);
        }
    }
}

void MapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(move_state == MIS_PRESSED) {
        emit(itemClicked());
    }
    else if(move_state == MIS_MOVED) {
        emit(itemMoved(mapToScene(event->pos() - pressPos)));
    }
    move_state = MIS_IDLE;
}
