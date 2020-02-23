#include "waypointitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "AircraftManager.h"


WaypointItem::WaypointItem(Point2DLatLon pt, int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, map, neutral_scale_zoom, parent),
    latlon(pt)
{
    std::optional<QColor> colorOption = AircraftManager::get()->getColor(ac_id);
    if(!colorOption.has_value()) {
        throw std::runtime_error("AcId not found!");
    }
    QColor color = colorOption.value();
    int size = qApp->property("WAYPOINTS_SIZE").toInt();

    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    point = new GraphicsPoint(size, color, this);
    QList<QColor> color_variants = makeColorVariants(color);
    point->setColors(color_variants[0], color_variants[1], color_variants[2]);
    point->setPos(scene_pos);
    point->setZValue(z_value);
    map->scene()->addItem(point);
    setZoomFactor(1.1);

    connect(
        point, &GraphicsPoint::pointMoved, this,
        [=](QPointF scenePos) {
            latlon = latlonPoint(scenePos, zoomLevel(map->zoom()), map->tileSize());
            emit(waypointMoved(latlon));
        }
    );

    connect(
        point, &GraphicsPoint::objectClicked, this,
        [=](QPointF scene_pos) {
            emit(itemClicked(scene_pos));
        }
    );

    connect(
        point, &GraphicsPoint::objectGainedHighlight, this,
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

void WaypointItem::setForbidHighlight(bool fh) {
    point->setForbidHighlight(fh);
}

void WaypointItem::setEditable(bool ed) {
    point->setEditable(ed);
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
