#include "waypoint_item.h"
#include "math.h"
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "AircraftManager.h"
#include "coordinatestransform.h"
#include "gcs_utils.h"

// create WaypointItem from position -> create corresponding Waypoint.
WaypointItem::WaypointItem(Point2DLatLon pt, QString ac_id, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, neutral_scale_zoom, parent), moving(false), m_size(0)
{
    original_waypoint = new Waypoint("", 0, pt, 0, this);
    init();
}

WaypointItem::WaypointItem(Point2DLatLon pt, QString ac_id, PprzPalette palette, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, palette, neutral_scale_zoom, parent), moving(false), m_size(0)
{
    original_waypoint = new Waypoint("", 0, pt, 0, this);
    init();
}

// create WaypointItem based on existing Waypoint
WaypointItem::WaypointItem(Waypoint* wp, QString ac_id, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, neutral_scale_zoom, parent), original_waypoint(wp), moving(false), m_size(0)
{
    init();
}

void WaypointItem::init() {
    auto settings = getAppSettings();
    z_value_highlighted = settings.value("map/z_values/highlighted").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/unhighlighted").toDouble();
    z_value = z_value_unhighlighted;

    _waypoint = new Waypoint(original_waypoint, this);
    m_size = settings.value("map/waypoint/size").toInt();

    point = new GraphicsPoint(m_size, palette, this);
    point->setZValue(z_value);


    graphics_text = new GraphicsText(original_waypoint->getName(), QColor(Qt::white), this);
    graphics_text->setZValue(z_value);


    setZoomFactor(1.1);


    connect(
        point, &GraphicsPoint::objectClicked, this,
        [=,this](QPointF scene_pos) {
            emit itemClicked(scene_pos);
        }
    );

    connect(
        point, &GraphicsPoint::objectDoubleClicked, this,
        [=,this](QPointF scene_pos) {
            emit itemDoubleClicked(scene_pos);
        }
    );

    connect(
        point, &GraphicsPoint::objectGainedHighlight, this,
        [=,this]() {
            setHighlighted(true);
            emit itemGainedHighlight();
        }
    );
}

void WaypointItem::addToMap(MapWidget* map) {
    map->scene()->addItem(point);
    map->scene()->addItem(graphics_text);
    // the name can be grabbed too: dragged, clicked and double clicked as the point
    graphics_text->installSceneEventFilter(point);
    graphics_text->setAcceptHoverEvents(true);
    // the mouse over the point or its name (the map marks what is linked to it)
    connect(point, &GraphicsObject::hoverChanged, this, [=,this](bool h) { emit waypointHovered(h, false); });
    connect(point, &GraphicsPoint::nameHoverChanged, this, [=,this](bool h) { emit waypointHovered(h, true); });
    connect(point, &GraphicsPoint::pointPressed, this, &WaypointItem::waypointPressed);

    connect(
        point, &GraphicsPoint::pointMoved, this,
        [=,this](QPointF scene_pos) {
            moving = true;
            // reverse position: from scene (mouse) to WGS84.
            Point2DLatLon latlon = CoordinatesTransform::get()->wgs84_from_scene(scene_pos, zoomLevel(map->zoom()), map->tileSize());
            this->setPosition(latlon);
        }
    );

    connect(
        point, &GraphicsPoint::pointMoveFinished, this,
        [=,this](QPointF scene_pos) {
            Point2DLatLon latlon = CoordinatesTransform::get()->wgs84_from_scene(scene_pos, zoomLevel(map->zoom()), map->tileSize());
            this->setPosition(latlon);
            emit waypointMoveFinished();
        }
    );
}

void WaypointItem::setHighlighted(bool h) {
    MapItem::setHighlighted(h);
    point->setHighlighted(h);
    graphics_text->setHighlighted(h);

    updateZValue();
}

void WaypointItem::setForbidHighlight(bool fh) {
    point->setForbidHighlight(fh);
}

void WaypointItem::setEditable(bool ed) {
    point->setEditable(ed);
    point->setIgnoreEvent(!ed);
}

void WaypointItem::updateZValue() {
    point->setZValue(z_value + (raised ? 0.5 : 0.));
    graphics_text->setZValue(z_value + (raised ? 0.5 : 0.));
}

void WaypointItem::setLabelLeader(QLineF scene_line) {
    if(scene_line.isNull()) {
        if(label_leader != nullptr) {
            label_leader->setVisible(false);
        }
        return;
    }
    if(label_leader == nullptr) {
        label_leader = new QGraphicsLineItem(graphics_text);
        QPen pen(QColor(255, 255, 255, 230), 1.5);
        pen.setCosmetic(true);
        label_leader->setPen(pen);
    }
    label_leader->setLine(QLineF(graphics_text->mapFromScene(scene_line.p1()),
                                 graphics_text->mapFromScene(scene_line.p2())));
    label_leader->setVisible(true);
}

void WaypointItem::setDimmed(bool d) {
    graphics_text->setOpacity(d ? 0.6 : 1.0);    // its line too (a child of the name)
}

void WaypointItem::setHoverMark(bool h) {
    graphics_text->setHoverMark(h);
    point->setHoverMark(h);
    graphics_text->setZValue(z_value + (h ? 1.0 : raised ? 0.5 : 0.));
    if(label_leader != nullptr) {
        QPen pen = label_leader->pen();
        pen.setColor(h ? QColor(0x4da3ff) : QColor(255, 255, 255, 230));
        pen.setWidthF(h ? 2.0 : 1.5);
        label_leader->setPen(pen);
    }
}

void WaypointItem::setRaised(bool r) {
    raised = r;
    updateZValue();
}

void WaypointItem::setVisible(bool visible) {
    point->setVisible(visible);
    graphics_text->setVisible(visible);
}

void WaypointItem::updateGraphics(MapWidget* map, uint32_t update_event) {
    if(update_event & (UpdateEvent::ITEM_CHANGED | UpdateEvent::MAP_ZOOMED | UpdateEvent::MAP_ROTATED)) {
        QPointF scene_pos = scenePoint(Point2DLatLon(_waypoint), zoomLevel(map->zoom()), map->tileSize());
        double s = getScale(map->zoom(), map->scaleFactor());
        double r = map->getRotation();
        point->setPos(scene_pos);
        point->setScale(s);
        point->setRotation(-r);

        auto rot = QTransform().rotate(-r);
        graphics_text->setPlainText(_waypoint->getName());
        graphics_text->setPos(scene_pos + rot.map(label_offset * s));
        graphics_text->setScale(s);
        graphics_text->setRotation(-r);
    }
}

QGraphicsItem* WaypointItem::marker() {
    return point;
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
    emit itemChanged();
    emit waypointMoved(ll);
}

void WaypointItem::update() {
    _waypoint->setLat(original_waypoint->getLat());
    _waypoint->setLon(original_waypoint->getLon());
    _waypoint->setAlt(original_waypoint->getAlt());
    _waypoint->setName(original_waypoint->getName());
    emit itemChanged();
}

void WaypointItem::commitPosition() {
    original_waypoint->setLat(_waypoint->getLat());
    original_waypoint->setLon(_waypoint->getLon());
    original_waypoint->setAlt(_waypoint->getAlt());
    emit itemChanged();
}

void WaypointItem::setStyle(GraphicsPoint::Style s){
    point->setStyle(s);
    graphics_text->setStyle(s);
    requestUpdate();
}

void WaypointItem::setAnimation(GraphicsObject::Animation a) {
    point->setAnimation(a);
}

void WaypointItem::setAnimate(bool animate) {
    if(animate) {
        point->setAnimation(GraphicsObject::Animation::WP_MOVING);
    } else {
        point->setAnimation(GraphicsObject::Animation::NONE);
    }
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
