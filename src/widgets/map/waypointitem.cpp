#include "waypointitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>


WaypointItem::WaypointItem(Point2DLatLon pt, int size, QColor color, int tile_size, double zoom, qreal z_value, double neutral_scale_zoom, QObject *parent) :
    MapItem(zoom, tile_size, z_value, neutral_scale_zoom, parent),
    latlon(pt)
{
    QPointF scene_pos = scenePoint(latlon, zoomLevel(zoom), tile_size);
    point = new GraphicsPoint(size, color);
    QList<QColor> color_variants = makeColorVariants(color);
    point->setColors(color_variants[0], color_variants[1], color_variants[2]);
    point->setPos(scene_pos);
    point->setZValue(z_value);
    setZoomFactor(1.1);

    connect(
        point, &GraphicsPoint::pointMoved,
        [=](QPointF scenePos) {
            latlon = latlonPoint(scenePos, zoomLevel(_zoom), tile_size);
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

}

void WaypointItem::setHighlighted(bool h) {
    highlighted = h;
    point->setHighlighted(h);
}

void WaypointItem::add_to_scene(QGraphicsScene* scene) {
    scene->addItem(point);
}

void WaypointItem::setZValue(qreal z) {
    z_value = z;
    point->setZValue(z);
}

void WaypointItem::updateGraphics() {
    QPointF scene_pos = scenePoint(latlon, zoomLevel(_zoom), tile_size);
    point->setPos(scene_pos);

    double s = getScale();
    point->setScale(s);
}

void WaypointItem::setPosition(Point2DLatLon ll) {
    latlon = ll;
    QPointF scene_pos = scenePoint(latlon, zoomLevel(_zoom), tile_size);
    point->setPos(scene_pos);
    emit(waypointMoved(latlon));
}
