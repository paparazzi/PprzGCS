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

    graphics_aircraft = new GraphicsAircraft(aircraft.getColor(), aircraft.getIcon(), size);
    graphics_text = new QGraphicsTextItem(aircraft.name());
    setZoomFactor(1.1);
    updateGraphics();
    map->scene()->addItem(graphics_aircraft);
    map->scene()->addItem(graphics_text);
}

void AircraftItem::updateGraphics() {
    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    graphics_aircraft->setPos(scene_pos);
    double s = getScale();
    graphics_aircraft->setScale(s);
    graphics_aircraft->setRotation(heading);
}

void AircraftItem::setPosition(Point2DLatLon pt) {
    latlon = pt;
    updateGraphics();
}

void AircraftItem::setHeading(double h) {
    heading = h;
    updateGraphics();
}

void AircraftItem::setHighlighted(bool h) {
    graphics_aircraft->setHighlighted(h);
}

void AircraftItem::setZValue(qreal z) {
    graphics_aircraft->setZValue(z);
}

void AircraftItem::setForbidHighlight(bool fh) {
    graphics_aircraft->setForbidHighlight(fh);
}

void AircraftItem::removeFromScene() {
    assert(graphics_aircraft != nullptr);
    map->scene()->removeItem(graphics_aircraft);
    delete graphics_aircraft;
}
