#include "circle_item.h"
#include <QPen>
#include <cmath>
#include <QApplication>
#include <QDebug>
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "map_item.h"
#include "AircraftManager.h"
#include "gcs_utils.h"

CircleItem::CircleItem(WaypointItem* center, double radius, QString ac_id, double neutral_scale_zoom):
  MapItem(ac_id, neutral_scale_zoom),
  center(center), _radius(radius), own_center(false)
{
    auto settings = getAppSettings();
    stroke = settings.value("map/circle/stroke").toInt();
    z_value_highlighted = settings.value("map/z_values/highlighted").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/unhighlighted").toDouble();
    z_value = z_value_unhighlighted;
    init(center);
}

void CircleItem::init(WaypointItem* center) {
    Aircraft aircraft = AircraftManager::get()->getAircraft(ac_id);

    circle = new GraphicsCircle(0, aircraft.getColor(), stroke, this);
    circle->setPos(center->scenePos());
    circle->setZValue(center->zValue() + 0.5);

    QList<QColor> color_variants = makeColorVariants(aircraft.getColor());
    circle->setColors(color_variants[0], color_variants[1], color_variants[2]);



    // dependence over center: if center changed, so do the CircleItem.
    connect(
        center, &WaypointItem::itemChanged, this,
        [=]() {
            emit itemChanged();
        }
    );

    connect(
        circle, &GraphicsCircle::objectClicked, this,
        [=](QPointF scene_pos) {
            qDebug() << "circle clicked at " << scene_pos;
        }
    );

    connect(
        circle, &GraphicsCircle::objectGainedHighlight, this,
        [=]() {
            setHighlighted(true);
            emit itemGainedHighlight();
        }
    );

    connect(
        center, &MapItem::itemGainedHighlight, this,
        [=]() {
            setHighlighted(true);
            emit itemGainedHighlight();
        }
    );
}

void CircleItem::addToMap(MapWidget* map) {
    map->scene()->addItem(circle);

    connect(
        circle, &GraphicsCircle::circleScaled, this,
        [=](qreal size) {
            _radius = distTile2Meters(circle->pos().y()/map->tileSize(), size/map->tileSize(), zoomLevel(map->zoom()));
            circle->setText(QString::number(static_cast<int>(_radius)) + "m");
            emit circleScaled(_radius);
        }
    );
}


void CircleItem::setHighlighted(bool h) {
    MapItem::setHighlighted(h);
    center->setHighlighted(h);
    circle->setHighlighted(h);

    updateZValue();
}

void CircleItem::setForbidHighlight(bool fh) {
    center->setForbidHighlight(fh);
    circle->setForbidHighlight(fh);
}

void CircleItem::setEditable(bool ed) {
    center->setEditable(ed);
    circle->setEditable(ed);
}

void CircleItem::updateZValue() {
    //the circle is above the waypoint
    center->updateZValue();
    circle->setZValue(z_value+0.5);
}

void CircleItem::updateGraphics(MapWidget* map) {
    double s = getScale(map->zoom(), map->scaleFactor());
    circle->setScaleFactor(s);

    QPointF scene_pos = scenePoint(center->position(), zoomLevel(map->zoom()), map->tileSize());
    circle->setPos(scene_pos);

    double pixelRadius = distMeters2Tile(_radius, center->position().lat(), zoomLevel(map->zoom()))*map->tileSize();
    circle->setRadius(pixelRadius);
}

void CircleItem::removeFromScene(MapWidget* map) {
    map->scene()->removeItem(circle);
    delete circle;
    if(own_center) {        // remove the waypoint only if it owns it.
        map->removeItem(center);
        center = nullptr;
    }
}

void CircleItem::setRadius(double radius) {
    _radius = radius;

    emit itemChanged();
}

void CircleItem::setStyle(GraphicsCircle::Style s) {
    circle->setStyle(s);
    if(s == GraphicsCircle::Style::CURRENT_NAV) {
        center->setStyle(GraphicsObject::Style::CURRENT_NAV);
    }
}
