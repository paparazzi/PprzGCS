#include "quiver_item.h"
#include "gcs_utils.h"
#include "mapwidget.h"

#include <QtDebug>

QuiverItem::QuiverItem(Point2DLatLon pos, Point2DLatLon vpos, QString ac_id,double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, PprzPalette(Qt::red), neutral_scale_zoom, parent),
    latlon(pos),
    vlatlon(vpos)
{
    auto settings = getAppSettings();

    z_value_highlighted = settings.value("map/z_values/shapes").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/shapes").toDouble();
    z_value = z_value_unhighlighted;

    CoordinatesTransform::get()->distance_azimut(latlon, vlatlon, distance, azimut);
    graphics_quiver = new GraphicsQuiver(distance, palette, this);

    graphics_quiver->setZValue(z_value);
    
    setZoomFactor(1.1);
}

void QuiverItem::addToMap(MapWidget* map) {
    map->scene()->addItem(graphics_quiver);
}

void QuiverItem::updateGraphics(MapWidget* map, uint32_t update_event) {
    if(update_event & (UpdateEvent::ITEM_CHANGED | UpdateEvent::MAP_ZOOMED)) {
        QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
        double size = distMeters2Tile(distance*map->tileSize(), latlon.lat(), zoomLevel(map->zoom()));
        double scale = size/graphics_quiver->size;

        graphics_quiver->setPos(scene_pos);
        graphics_quiver->setScale(scale);
        graphics_quiver->setRotation(azimut);
    }
}

void QuiverItem::setPos(Point2DLatLon pos) {
    latlon = pos;
    CoordinatesTransform::get()->distance_azimut(latlon, vlatlon, distance, azimut);
    emit itemChanged();
}

void QuiverItem::setVec(Point2DLatLon vpos) {
    vlatlon = vpos;
    CoordinatesTransform::get()->distance_azimut(latlon, vlatlon, distance, azimut);
    emit itemChanged();
}

void QuiverItem::setPosVec(Point2DLatLon pos, Point2DLatLon vpos) {
    latlon = pos;
    vlatlon = vpos;
    CoordinatesTransform::get()->distance_azimut(latlon, vlatlon, distance, azimut);
    emit itemChanged();
}

void QuiverItem::removeFromScene(MapWidget* map) {
    assert(graphics_quiver != nullptr);
    map->scene()->removeItem(graphics_quiver);
    delete graphics_quiver;
}

void QuiverItem::setForbidHighlight(bool fh) {
    graphics_quiver->setForbidHighlight(fh);
}

void QuiverItem::setEditable(bool ed) {
    (void)ed;
}

void QuiverItem::updateZValue() {
    graphics_quiver->setZValue(z_value);
}