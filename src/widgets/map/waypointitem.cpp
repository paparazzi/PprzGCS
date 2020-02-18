#include "waypointitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"


WaypointItem::WaypointItem(Point2DLatLon pt, int size, QColor color, int tile_size, double neutral_scale_zoom, QGraphicsItem *parent) :
    MapItem(tile_size, neutral_scale_zoom, parent),
    move_state(MIS_IDLE), movable(true), latlon(pt)
{
    ellipse = new QGraphicsEllipseItem(- size/2,- size/2, size, size);
    ellipse->setBrush(QBrush(color));
    addToGroup(ellipse);
}

void WaypointItem::scaleToZoom(double zoom, double viewScale) {
    _zoom = zoom;
    QPointF point = scenePoint(latlon, zoomLevel(zoom), tile_size);
    setPos(point);

    double s = pow(zoom_factor, zoom - neutral_scale_zoom)/viewScale;
    setScale(s);
}

void WaypointItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    move_state = MIS_PRESSED;
}

void WaypointItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
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

void WaypointItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(move_state == MIS_PRESSED) {
        emit(itemClicked());
    }
    else if(move_state == MIS_MOVED) {
        setPosition(latlonPoint(mapToScene(event->pos() - pressPos), zoomLevel(_zoom), tile_size));
        emit(waypointMoved(latlon));
    }
    move_state = MIS_IDLE;
}
