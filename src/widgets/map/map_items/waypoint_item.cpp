#include "waypoint_item.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "AircraftManager.h"
#include "coordinatestransform.h"

// create WaypointItem from position -> create corresponding Waypoint.
WaypointItem::WaypointItem(Point2DLatLon pt, QString ac_id, qreal z_value, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, neutral_scale_zoom, parent), moving(false)
{
    original_waypoint = make_shared<Waypoint>("", 0, pt.lat(), pt.lon(), 0);
    init();
}

// create WaypointItem based on existing Waypoint
WaypointItem::WaypointItem(shared_ptr<Waypoint> wp, QString ac_id, qreal z_value, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, neutral_scale_zoom, parent), original_waypoint(wp), moving(false)
{
    init();
}

void WaypointItem::init() {
    _waypoint = make_shared<Waypoint>(*original_waypoint);
    Aircraft aircraft = AircraftManager::get()->getAircraft(ac_id);
    int size = qApp->property("WAYPOINTS_SIZE").toInt();
    name = original_waypoint->getName().c_str();
    point = new GraphicsPoint(size, aircraft.getColor(), this);
    QList<QColor> color_variants = makeColorVariants(aircraft.getColor());
    point->setColors(color_variants[0], color_variants[1], color_variants[2]);
    point->setZValue(z_value);


    graphics_text = new QGraphicsTextItem(name);
    //graphics_text->setDefaultTextColor(aircraft.getColor());
    graphics_text->setDefaultTextColor(Qt::white);
    graphics_text->setZValue(z_value);


    setZoomFactor(1.1);


    connect(
        point, &GraphicsPoint::objectClicked, this,
        [=](QPointF scene_pos) {
            emit(itemClicked(scene_pos));
        }
    );

    connect(
        point, &GraphicsPoint::objectDoubleClicked, this,
        [=](QPointF scene_pos) {
            emit(itemDoubleClicked(scene_pos));
        }
    );

    connect(
        point, &GraphicsPoint::objectGainedHighlight, this,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );
}

void WaypointItem::addToMap(MapWidget* map) {
    map->scene()->addItem(point);
    map->scene()->addItem(graphics_text);

    connect(
        point, &GraphicsPoint::pointMoved, this,
        [=](QPointF scene_pos) {
            moving = true;
            // reverse position: from scene (mouse) to WGS84.
            Point2DLatLon latlon = CoordinatesTransform::get()->wgs84_from_scene(scene_pos, zoomLevel(map->zoom()), map->tileSize());
            this->setPosition(latlon);
        }
    );

    connect(
        point, &GraphicsPoint::pointMoveFinished, this,
        [=](QPointF scene_pos) {
            Point2DLatLon latlon = CoordinatesTransform::get()->wgs84_from_scene(scene_pos, zoomLevel(map->zoom()), map->tileSize());
            this->setPosition(latlon);
            emit(waypointMoveFinished());
        }
    );
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

void WaypointItem::updateGraphics(double zoom, double scale_factor, int tile_size) {
    QPointF scene_pos = scenePoint(Point2DLatLon(_waypoint), zoomLevel(zoom), tile_size);
    double s = getScale(zoom, scale_factor);
    point->setPos(scene_pos);
    point->setScale(s);

    graphics_text->setPos(scene_pos + QPointF(5*s, -30*s));
    graphics_text->setScale(s);
}

void WaypointItem::removeFromScene(MapWidget* map) {
    map->scene()->removeItem(point);
    map->scene()->removeItem(graphics_text);
    delete point;
    delete graphics_text;
}

void WaypointItem::setPosition(Point2DLatLon ll) {
    _waypoint->setLat(ll.lat());
    _waypoint->setLon(ll.lon());
    emit(itemChanged());
    emit(waypointMoved(ll));
}

void WaypointItem::updatePosition() {
    _waypoint->setLat(original_waypoint->getLat());
    _waypoint->setLon(original_waypoint->getLon());
    _waypoint->setAlt(original_waypoint->getAlt());
    emit(itemChanged());
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
