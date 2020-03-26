#include "waypoint_item.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "AircraftManager.h"

WaypointItem::WaypointItem(Point2DLatLon pt, QString ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, map, neutral_scale_zoom, parent), moving(false)
{
    waypoint = make_shared<Waypoint>("", 0, pt.lat(), pt.lon(), 0);
    init();
}

WaypointItem::WaypointItem(shared_ptr<Waypoint> wp, QString ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, map, neutral_scale_zoom, parent), waypoint(wp), moving(false)
{
    init();
}

void WaypointItem::init() {
    Aircraft aircraft = AircraftManager::get()->getAircraft(ac_id);
    int size = qApp->property("WAYPOINTS_SIZE").toInt();
    name = waypoint->getName().c_str();
    QPointF scene_pos = scenePoint(Point2DLatLon(waypoint), zoomLevel(map->zoom()), map->tileSize());
    point = new GraphicsPoint(size, aircraft.getColor(), this);
    QList<QColor> color_variants = makeColorVariants(aircraft.getColor());
    point->setColors(color_variants[0], color_variants[1], color_variants[2]);
    point->setPos(scene_pos);
    point->setZValue(z_value);
    map->scene()->addItem(point);

    graphics_text = new QGraphicsTextItem(name);
    //graphics_text->setDefaultTextColor(aircraft.getColor());
    graphics_text->setDefaultTextColor(Qt::white);
    graphics_text->setZValue(z_value);
    map->scene()->addItem(graphics_text);

    setZoomFactor(1.1);

    connect(
        point, &GraphicsPoint::pointMoved, this,
        [=](QPointF scene_pos) {
            moving = true;
            Point2DLatLon latlon = latlonPoint(scene_pos, zoomLevel(map->zoom()), map->tileSize());
            graphics_text->setPos(scene_pos + QPointF(10, 10));
            emit(waypointMoved(latlon));
        }
    );

    connect(
        point, &GraphicsPoint::pointMoveFinished, this,
        [=](QPointF scene_pos) {
            moving = false;
            Point2DLatLon latlon = latlonPoint(scene_pos, zoomLevel(map->zoom()), map->tileSize());
            waypoint->setLat(latlon.lat());
            waypoint->setLon(latlon.lon());
            emit(waypointMoveFinished(latlon));
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
    graphics_text->setVisible(h);
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
    graphics_text->setZValue(z);
}

void WaypointItem::updateGraphics() {
    QPointF scene_pos = scenePoint(Point2DLatLon(waypoint), zoomLevel(map->zoom()), map->tileSize());
    double s = getScale();
    point->setPos(scene_pos);
    point->setScale(s);

    graphics_text->setPos(scene_pos + QPointF(10, 10));
    graphics_text->setScale(s);
}

void WaypointItem::removeFromScene() {
    map->scene()->removeItem(point);
    map->scene()->removeItem(graphics_text);
    delete point;
    delete graphics_text;
}

void WaypointItem::setPosition(Point2DLatLon ll) {
    waypoint->setLat(ll.lat());
    waypoint->setLon(ll.lon());
    QPointF scene_pos = scenePoint(ll, zoomLevel(map->zoom()), map->tileSize());
    point->setPos(scene_pos);
    graphics_text->setPos(scene_pos + QPointF(10, 10));
    emit(waypointMoved(ll));
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
