#include "quiver_item.h"
#include "gcs_utils.h"
#include "mapwidget.h"

QuiverItem::QuiverItem(Point2DLatLon pos, Point2DLatLon vec, double neutral_scale_zoom, QObject *parent) :
    MapItem("__NO_AC__", PprzPalette(Qt::red), neutral_scale_zoom, parent),
    latlon(pos)
{
    auto settings = getAppSettings();

    z_value_highlighted = settings.value("map/z_values/shapes").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/shapes").toDouble();
    z_value = z_value_unhighlighted;

    // TODO: "size" y "course" vienen determinados por "pos" y "vec".
    size = 100;
    course = 10;

    graphics_quiver = new GraphicsQuiver(size, palette, this);
    graphics_quiver->setZValue(z_value);

    setZoomFactor(1.1);
}

void QuiverItem::addToMap(MapWidget* map) {
    map->scene()->addItem(graphics_quiver);
}

void QuiverItem::updateGraphics(MapWidget* map, uint32_t update_event) {
    if(update_event & (UpdateEvent::ITEM_CHANGED | UpdateEvent::MAP_ZOOMED)) {
        QPointF scene_pos = scenePoint(latlon, zoomLevel(map->zoom()), map->tileSize());
        graphics_quiver->setPos(scene_pos);
        double s = getScale(map->zoom(), map->scaleFactor());
        graphics_quiver->setScale(s);
        graphics_quiver->setRotation(course);
    }
}

void QuiverItem::setPosition(Point2DLatLon pos) {
    latlon = pos;
    emit itemChanged();
}

void QuiverItem::setVector(Point2DLatLon vec) {
    course = 10;  // TODO: "size" y "course" vienen determinados por "pos" y "vec".
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