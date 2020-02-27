#include "aircraft_item.h"
#include "mapwidget.h"
#include "AircraftManager.h"
#include <QApplication>
#include <QDebug>
#include "aircraft.h"

AircraftItem::AircraftItem(Point2DLatLon pt, QString ac_id, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, 200, map, neutral_scale_zoom, parent),
    latlon(pt)
{
    std::optional<Aircraft> aircraftOption = AircraftManager::get()->getAircraft(ac_id);
    if(!aircraftOption.has_value()) {
        throw std::runtime_error("AcId not found!");
    }
    Aircraft aircraft = aircraftOption.value();
    int size = qApp->property("AIRCRAFTS_SIZE").toInt();

    color_idle = aircraft.getColor();
    color_unfocused = labelUnfocusedColor(aircraft.getColor());

    QList<QColor> color_variants = makeColorVariants(aircraft.getColor());

    graphics_aircraft = new GraphicsAircraft(aircraft.getColor(), aircraft.getIcon(), size);
    graphics_text = new QGraphicsTextItem(aircraft.name());
    graphics_text->setDefaultTextColor(aircraft.getColor());
    graphics_track = new GraphicsTrack(aircraft.getColor(), trackUnfocusedColor(aircraft.getColor()));

    setZoomFactor(1.1);
    updateGraphics();
    map->scene()->addItem(graphics_aircraft);
    map->scene()->addItem(graphics_text);
    map->scene()->addItem(graphics_track);

    map->addItem(this);
}

void AircraftItem::updateGraphics() {
    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    graphics_aircraft->setPos(scene_pos);
    double s = getScale();
    graphics_aircraft->setScale(s);
    graphics_aircraft->setRotation(heading);

    graphics_text->setScale(s);
    graphics_text->setPos(scene_pos + QPointF(10, 10));

    QPolygonF scenePoints;
    for(auto ll: track) {
        scenePoints.append(scenePoint(ll, zoomLevel(map->zoom()), map->tileSize()));
    }
    graphics_track->setPoints(scenePoints);
}

void AircraftItem::setPosition(Point2DLatLon pt) {
    latlon = pt;
    track.append(pt);
    updateGraphics();
}

void AircraftItem::setHeading(double h) {
    heading = h;
    updateGraphics();
}

void AircraftItem::setHighlighted(bool h) {
    graphics_aircraft->setHighlighted(h);
    graphics_track->setHighlighted(h);
    if(h) {
        graphics_text->setDefaultTextColor(color_idle);
    } else {
        graphics_text->setDefaultTextColor(color_unfocused);
    }
}

void AircraftItem::setZValue(qreal z) {
    //aircrafts are on top of everything else
    graphics_aircraft->setZValue(z + 100);
    graphics_text->setZValue(z);
    graphics_track->setZValue(z);
}

void AircraftItem::setForbidHighlight(bool fh) {
    graphics_aircraft->setForbidHighlight(fh);
}

void AircraftItem::removeFromScene() {
    assert(graphics_aircraft != nullptr);
    map->scene()->removeItem(graphics_aircraft);
    delete graphics_aircraft;
}
