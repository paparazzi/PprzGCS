#include "waypointitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"


WaypointItem::WaypointItem(Point2DLatLon pt, int size, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(z_value, map, neutral_scale_zoom, parent),
    latlon(pt)
{
    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    point = new GraphicsPoint(size, color, this);
    QList<QColor> color_variants = makeColorVariants(color);
    point->setColors(color_variants[0], color_variants[1], color_variants[2]);
    point->setPos(scene_pos);
    point->setZValue(z_value);
    map->scene()->addItem(point);
    setZoomFactor(1.1);

    connect(
        point, &GraphicsPoint::pointMoved,
        [=](QPointF scenePos) {
            latlon = latlonPoint(scenePos, zoomLevel(map->zoom()), map->tileSize());
            emit(waypointMoved(latlon));
        }
    );

    connect(
        point, &GraphicsPoint::objectClicked,
        [=](QPointF scene_pos) {
            emit(itemClicked(scene_pos));
        }
    );

    connect(
        point, &GraphicsPoint::objectGainedHighlight,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );

    map->addItem(this);

}

void WaypointItem::setHighlighted(bool h) {
    highlighted = h;
    point->setHighlighted(h);
}

void WaypointItem::setZValue(qreal z) {
    z_value = z;
    point->setZValue(z);
}

void WaypointItem::updateGraphics() {
    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    point->setPos(scene_pos);

    double s = getScale();
    point->setScale(s);
}

void WaypointItem::removeFromScene() {
    map->scene()->removeItem(point);
    delete point;
}

void WaypointItem::setPosition(Point2DLatLon ll) {
    latlon = ll;
    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    point->setPos(scene_pos);
    emit(waypointMoved(latlon));
}

QPointF WaypointItem::scenePos() {
    if(point != nullptr) {
        return point->scenePos();
    }
    return QPointF(0,0);
}


void WaypointItem::setIgnoreEvent(bool ignore) {
    point->setIgnoreEvent(ignore);
}
