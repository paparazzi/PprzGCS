#include "gcs_utils.h"
#include "mapwidget.h"

// TODO: Se introduzca un Point2DLatLon o un QList<Point2DLatLon> (dos constructores distintos) se debe de generar una
QuiverItem::QuiverItem(Point2DLatLon pos, Point2DLatLon vpos, QString id, QPen pen, double neutral_scale_zoom, QObject *parent) :
    MapItem(id, PprzPalette(pen.color()), neutral_scale_zoom, parent)
{
    auto settings = getAppSettings();

    z_value_highlighted = settings.value("map/z_values/shapes").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/shapes").toDouble();
    z_value = z_value_unhighlighted;

    double distance, azimut;
    CoordinatesTransform::get()->distance_azimut(pos, vpos, distance, azimut);
    auto graphics_quiver = new GraphicsQuiver(distance, palette, QPen(Qt::red, 3), this);
    graphics_quiver->setZValue(z_value);

    graphics_quiver_l.append(graphics_quiver);
    latlon_l.append(pos);
    vlatlon_l.append(vpos);
    distance_l.append(distance);
    azimut_l.append(azimut);

    setZoomFactor(1.1);
}

QuiverItem::QuiverItem(QList<Point2DLatLon> pos, QList<Point2DLatLon> vpos, QString id, QPen pen, double neutral_scale_zoom, QObject *parent) :
    MapItem(id, PprzPalette(pen.color()), neutral_scale_zoom, parent)
{
    auto settings = getAppSettings();

    z_value_highlighted = settings.value("map/z_values/shapes").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/shapes").toDouble();
    z_value = z_value_unhighlighted;

    for (int i=0; i<pos.size(); i++) {
        double distance, azimut;
        CoordinatesTransform::get()->distance_azimut(pos[i], vpos[i], distance, azimut);
        auto graphics_quiver = new GraphicsQuiver(distance, palette, pen, this);
        graphics_quiver->setZValue(z_value);

        graphics_quiver_l.append(graphics_quiver);
        latlon_l.append(pos[i]);
        vlatlon_l.append(vpos[i]);
        distance_l.append(distance);
        azimut_l.append(azimut);
    }

    setZoomFactor(1.1);
}

void QuiverItem::addToMap(MapWidget* map) {
    foreach (GraphicsQuiver* graphics_quiver, graphics_quiver_l) {
        map->scene()->addItem(graphics_quiver);
    }
}

void QuiverItem::updateGraphics(MapWidget* map, uint32_t update_event) {
    if(update_event & (UpdateEvent::ITEM_CHANGED | UpdateEvent::MAP_ZOOMED)) {
        for (int i=0; i<graphics_quiver_l.size(); i++) {
        QPointF scene_pos = scenePoint(latlon_l[i], zoomLevel(map->zoom()), map->tileSize());
        double size = distMeters2Tile(distance_l[i]*map->tileSize(), latlon_l[i].lat(), zoomLevel(map->zoom()));
        double scale = size/graphics_quiver_l[i]->size;

        graphics_quiver_l[i]->setPos(scene_pos);
        graphics_quiver_l[i]->setScale(scale);
        graphics_quiver_l[i]->setRotation(azimut_l[i]);
        }
    }
}

void QuiverItem::removeFromScene(MapWidget* map) {
    foreach (GraphicsQuiver* graphics_quiver, graphics_quiver_l) {
        assert(graphics_quiver != nullptr);
        map->scene()->removeItem(graphics_quiver);
        delete graphics_quiver;
    }
}

void QuiverItem::setForbidHighlight(bool fh) {
    (void)fh;
}

void QuiverItem::setEditable(bool ed) {
    (void)ed;
}

void QuiverItem::updateZValue() {
    foreach (GraphicsQuiver* graphics_quiver, graphics_quiver_l) {
        graphics_quiver->setZValue(z_value);
    }
}
    