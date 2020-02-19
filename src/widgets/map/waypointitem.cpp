#include "waypointitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>


WaypointItem::WaypointItem(Point2DLatLon pt, int size, QColor color, int tile_size, double zoom, double neutral_scale_zoom, QObject *parent) :
    MapItem(zoom, tile_size, neutral_scale_zoom, parent),
    latlon(pt)
{
    QPointF scene_pos = scenePoint(latlon, zoomLevel(zoom), tile_size);
    point = new GraphicsPoint(size);
    point->setPos(scene_pos);
    point->setBrush(QBrush(color));
    point->setZValue(100);
    setZoomFactor(1.1);

    connect(
        point, &GraphicsPoint::pointMoved,
        [=](QPointF scenePos) {
            latlon = latlonPoint(scenePos, zoomLevel(_zoom), tile_size);
            emit(waypointMoved(latlon));
        }
    );

}

void WaypointItem::add_to_scene(QGraphicsScene* scene) {
    scene->addItem(point);
}

void WaypointItem::scaleToZoom(double zoom, double viewScale) {
    _zoom = zoom;
    QPointF scene_pos = scenePoint(latlon, zoomLevel(zoom), tile_size);
    point->setPos(scene_pos);

    double s = pow(zoom_factor, zoom - neutral_scale_zoom)/viewScale;
    point->setScale(s);
}

void WaypointItem::setPosition(Point2DLatLon ll) {
    latlon = ll;
    QPointF scene_pos = scenePoint(latlon, zoomLevel(_zoom), tile_size);
    point->setPos(scene_pos);
    emit(waypointMoved(latlon));
}
