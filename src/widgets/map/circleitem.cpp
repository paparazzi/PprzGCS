#include "circleitem.h"
#include <QPen>
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "mapitem.h"

CircleItem::CircleItem(Point2DLatLon pt, double radius, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(z_value, map, neutral_scale_zoom, parent),
    _radius(radius), stroke(5)
{
    center = new WaypointItem(pt, 20, color, z_value, map, neutral_scale_zoom, parent);
    center->setZoomFactor(1.1);
    init(center, radius, color, map);
}

CircleItem::CircleItem(WaypointItem* center, double radius, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom):
  MapItem(z_value, map, neutral_scale_zoom),
  center(center), _radius(radius), stroke(5)
{
    init(center, radius, color, map);
}

void CircleItem::init(WaypointItem* center, double radius, QColor color, MapWidget* map) {
    double pixelRadius = distMeters2Tile(radius, center->position().lat(), zoomLevel(map->zoom())) * map->tileSize();
    circle = new GraphicsCircle(pixelRadius, QPen(QBrush(color), stroke), this);
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

void CircleItem::setZValue(qreal z) {
    z_value = z;
    //the circle is above the waypoint
    center->setZValue(z-0.5);
    circle->setZValue(z);
}

void CircleItem::updateGraphics() {
    //double pixelRadius = distMeters2Tile(_radius, center->position().lat(), zoomLevel(map->zoom()))*map->tileSize();

    QPointF scene_pos = scenePoint(center->position(), zoomLevel(map->zoom()), map->tileSize());
    circle->setPos(scene_pos);

    setRadius(_radius);
    //circle->setRadius(pixelRadius);

    double s = getScale();

    QPen p = circle->pen();
    p.setWidth(static_cast<int>(stroke * s));
    circle->setPen(p);
}

void CircleItem::setRadius(double radius) {
    _radius = radius;
    double pixelRadius = distMeters2Tile(_radius, center->position().lat(), zoomLevel(map->zoom()))*map->tileSize();
    circle->setRadius(pixelRadius);
}
