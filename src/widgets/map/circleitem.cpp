#include "circleitem.h"
#include <QPen>
#include <cmath>
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "mapitem.h"
#include "AircraftManager.h"

CircleItem::CircleItem(Point2DLatLon pt, double radius, int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, map, neutral_scale_zoom, parent),
    _radius(radius)
{
    center = new WaypointItem(pt, 20, ac_id, z_value, map, neutral_scale_zoom, parent);
    center->setZoomFactor(1.1);
    stroke = qApp->property("CIRCLE_STROKE").toInt();
    init(center, radius);
}

CircleItem::CircleItem(WaypointItem* center, double radius, int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom):
  MapItem(ac_id, z_value, map, neutral_scale_zoom),
  center(center), _radius(radius)
{
    stroke = qApp->property("CIRCLE_STROKE").toInt();
    init(center, radius);
}

void CircleItem::init(WaypointItem* center, double radius) {
    std::optional<QColor> colorOption = AircraftManager::get()->getColor(ac_id);
    if(!colorOption.has_value()) {
        throw std::runtime_error("AcId not found!");
    }
    QColor color = colorOption.value();

    double pixelRadius = distMeters2Tile(radius, center->position().lat(), zoomLevel(map->zoom())) * map->tileSize();
    circle = new GraphicsCircle(pixelRadius, color, stroke, this);
    circle->setPos(center->scenePos());
    circle->setZValue(center->zValue() + 0.5);

    QList<QColor> color_variants = makeColorVariants(color);
    circle->setColors(color_variants[0], color_variants[1], color_variants[2]);

    map->scene()->addItem(circle);

    connect(
        center, &WaypointItem::waypointMoved,
        [=](Point2DLatLon latlon) {
            QPointF p = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
            circle->setPos(p);
            emit(circleMoved(latlon));
        }
    );

    connect(
        circle, &GraphicsCircle::circleScaled,
        [=](qreal size) {
            _radius = distTile2Meters(circle->pos().y()/map->tileSize(), size/map->tileSize(), zoomLevel(map->zoom()));
            emit(circleScaled(_radius));
        }
    );

    connect(
        circle, &GraphicsCircle::objectClicked,
        [=](QPointF scene_pos) {
            qDebug() << "circle clicked at " << scene_pos;
        }
    );

    connect(
        circle, &GraphicsCircle::objectGainedHighlight,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );

    connect(
        center, &MapItem::itemGainedHighlight,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );

    map->addItem(this);
}




void CircleItem::setHighlighted(bool h) {
    highlighted = h;
    center->setHighlighted(h);
    circle->setHighlighted(h);
}

void CircleItem::setForbidHighlight(bool fh) {
    center->setForbidHighlight(fh);
    circle->setForbidHighlight(fh);
}

void CircleItem::setEditable(bool ed) {
    center->setEditable(ed);
    circle->setEditable(ed);
}

void CircleItem::setZValue(qreal z) {
    z_value = z;
    //the circle is above the waypoint
    center->setZValue(z-0.5);
    circle->setZValue(z);
}

void CircleItem::updateGraphics() {
    //double pixelRadius = distMeters2Tile(_radius, center->position().lat(), zoomLevel(map->zoom()))*map->tileSize();
    double s = getScale();
    circle->setScaleFactor(s);

    QPointF scene_pos = scenePoint(center->position(), zoomLevel(map->zoom()), map->tileSize());
    circle->setPos(scene_pos);
    setRadius(_radius);
}

void CircleItem::removeFromScene() {
    map->scene()->removeItem(circle);
    delete circle;
    // do not remove the waypoint. It still lives !
}

void CircleItem::setRadius(double radius) {
    _radius = radius;
    double pixelRadius = distMeters2Tile(_radius, center->position().lat(), zoomLevel(map->zoom()))*map->tileSize();
    circle->setRadius(pixelRadius);
}
