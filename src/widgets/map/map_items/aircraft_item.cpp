#include "aircraft_item.h"
#include "mapwidget.h"
#include "AircraftManager.h"
#include <QApplication>
#include <QDebug>
#include "aircraft.h"

AircraftItem::AircraftItem(Point2DLatLon pt, QString ac_id, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, 200, neutral_scale_zoom, parent),
    latlon(pt), heading(0.), last_chunk_index(0)
{
    Aircraft aircraft = AircraftManager::get()->getAircraft(ac_id);
    int size = qApp->property("AIRCRAFTS_SIZE").toInt();

    color_idle = aircraft.getColor();
    color_unfocused = labelUnfocusedColor(aircraft.getColor());

    QList<QColor> color_variants = makeColorVariants(aircraft.getColor());

    graphics_aircraft = new GraphicsAircraft(aircraft.getColor(), aircraft.getIcon(), size);
    graphics_text = new GraphicsText(aircraft.name());
    graphics_text->setDefaultTextColor(aircraft.getColor());

    for(int i=0; i<qApp->property("TRACK_MAX_CHUNKS").toInt(); i++) {
        auto gt = new GraphicsTrack(aircraft.getColor(), trackUnfocusedColor(aircraft.getColor()));
        graphics_tracks.append(gt);
        QList<Point2DLatLon> l;
        track_chuncks.append(l);
    }

    setZoomFactor(1.1);
}

void AircraftItem::addToMap(MapWidget* map) {

    for(auto gt: graphics_tracks) {
        map->scene()->addItem(gt);
    }

    map->scene()->addItem(graphics_aircraft);
    map->scene()->addItem(graphics_text);
}

void AircraftItem::updateGraphics(MapWidget* map) {
    QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
    graphics_aircraft->setPos(scene_pos);
    double s = getScale(map->zoom(), map->scaleFactor());
    graphics_aircraft->setScale(s);
    graphics_aircraft->setRotation(heading);

    graphics_text->setScale(s);
    graphics_text->setPos(scene_pos + QPointF(10, 10));


    for(int i = 0; i<track_chuncks.size(); i++) {
        QPolygonF scenePoints;
        for(auto pt: track_chuncks[i]) {
            scenePoints.append(scenePoint(pt, zoomLevel(map->zoom()), map->tileSize()));
        }
        graphics_tracks[i]->setPoints(scenePoints);
    }
}

void AircraftItem::setPosition(Point2DLatLon pt) {
    latlon = pt;

    track_chuncks[last_chunk_index].append(pt);

    if(track_chuncks[last_chunk_index].size() >= qApp->property("TRACK_CHUNCK_SIZE").toInt()) {
        last_chunk_index = (last_chunk_index + 1) % track_chuncks.size();
        assert(track_chuncks[last_chunk_index].size() == 0 || track_chuncks[last_chunk_index].size() == 1);
        if(!track_chuncks[last_chunk_index].isEmpty()) {
            track_chuncks[last_chunk_index].clear();
        }
        track_chuncks[last_chunk_index].append(pt);
    }

    int first_chunk_index = (last_chunk_index + 1) % track_chuncks.size();
    if(!track_chuncks[first_chunk_index].isEmpty()) {
        track_chuncks[first_chunk_index].removeFirst();
    }

    emit(itemChanged());
}

void AircraftItem::clearTrack() {
    for(auto tc : track_chuncks) {
        tc.clear();
        last_chunk_index = 0;
    }
    emit(itemChanged());
}

void AircraftItem::setHeading(double h) {
    heading = h;
    emit(itemChanged());
}

void AircraftItem::setHighlighted(bool h) {
    graphics_aircraft->setHighlighted(h);
    for(auto gt: graphics_tracks) {
        gt->setHighlighted(h);
    }
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
    for(auto gt: graphics_tracks) {
        gt->setZValue(z);
    }

}

void AircraftItem::setForbidHighlight(bool fh) {
    graphics_aircraft->setForbidHighlight(fh);
}

void AircraftItem::removeFromScene(MapWidget* map) {
    assert(graphics_aircraft != nullptr);
    map->scene()->removeItem(graphics_aircraft);
    map->scene()->removeItem(graphics_text);
    for(auto gt: graphics_tracks) {
        map->scene()->removeItem(gt);
        delete gt;
    }
    graphics_tracks.clear();
    delete graphics_aircraft;
    delete graphics_text;
}
