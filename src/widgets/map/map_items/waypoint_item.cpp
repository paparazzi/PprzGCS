#include "waypoint_item.h"
#include "math.h"
#include <QDebug>
#include <QTextDocument>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "AircraftManager.h"
#include "srtm_manager.h"
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
    altitude_mode = settings.value("map/waypoint_altitude").toString();
    altitude_always = settings.value("map/waypoint_altitude_always").toString();

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

/// Its altitude, as set: MSL, or the height above the ground: "AGL" above the terrain (SRTM), "H"
/// above the flight plan ground altitude when there is no SRTM data there (the height of Paparazzi)
QString WaypointItem::altitudeText() {
    double msl = _waypoint->getAlt();
    QString height;
    if(AircraftManager::get()->aircraftExists(acId())) {
        auto fp = AircraftManager::get()->getAircraft(acId())->getFlightPlan();
        auto ele = SRTMManager::get()->get_elevation(_waypoint->getLat(), _waypoint->getLon());
        double h = msl - (ele ? ele.value() : fp->getGroundAlt());
        height = QString("%1 %2 m").arg(ele ? "AGL" : "H").arg(h, 0, 'f', 0);
    }
    // one value, as set (on hover and always the same)
    if(altitude_always == "MSL" || height.isEmpty()) {
        return QString("MSL %1 m").arg(msl, 0, 'f', 0);
    }
    return height;
}

void WaypointItem::placeHoverInfo() {
    if(hover_info == nullptr || !hover_info->isVisible()) {
        return;
    }
    // under the name, aligned on the side of the name away from the waypoint; moved sideways
    // where it would touch the waypoint (a name above its waypoint)
    double m = graphics_text->document()->documentMargin();
    QRectF tr = graphics_text->boundingRect();
    double w = hover_info->boundingRect().width();
    bool left = label_offset.x() < 0;
    double x = left ? tr.width() - m - w : m;
    // right under the text of the name (its box has a margin and the line spacing below it)
    double y = tr.height() - m - QFontMetricsF(graphics_text->font()).descent() - 1;
    hover_info->setPos(x, y);
    QRectF marker = graphics_text->mapFromScene(point->sceneBoundingRect()).boundingRect();
    QRectF info = hover_info->mapRectToParent(hover_info->boundingRect());
    if(info.intersects(marker)) {
        x += left ? marker.left() - info.right() - 2 : marker.right() - info.left() + 2;
        hover_info->setPos(x, y);
    }
    QString t = hover_info->text();
    hover_info->setToolTip(t.contains("AGL") ? "AGL: above the terrain (SRTM)" :
                           t.contains("H ") ? "H: above the flight plan ground altitude (no SRTM data here)" :
                                              "MSL: above sea level");
}

void WaypointItem::setHoverInfo(QString text) {
    if(text.isEmpty()) {
        if(hover_info != nullptr) {
            hover_info->setVisible(false);
        }
        return;
    }
    if(hover_info == nullptr) {
        hover_info = new QGraphicsSimpleTextItem(graphics_text);
        QFont f = graphics_text->font();
        f.setPointSizeF(f.pointSizeF() * 0.8);
        hover_info->setFont(f);
        hover_info->setBrush(QColor(0xcfe6ff));
    }
    hover_info->setText(text);
    hover_info->setVisible(true);
    placeHoverInfo();
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
        if(altitude_mode == "Always" && (update_event & UpdateEvent::ITEM_CHANGED)) {
            setHoverInfo(altitudeText());
        }
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
