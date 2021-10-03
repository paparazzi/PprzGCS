#include "intruder_item.h"
#include "gcs_utils.h"
#include "mapwidget.h"

IntruderItem::IntruderItem(QString name, Point2DLatLon pt, double course, double neutral_scale_zoom, QObject *parent) :
    MapItem("__NO_AC__", PprzPalette(Qt::red), neutral_scale_zoom, parent),
    name(name), latlon(pt), course(course)
{
    auto settings = getAppSettings();

    z_value_highlighted = settings.value("map/z_values/shapes").toDouble();
    z_value_unhighlighted = settings.value("map/z_values/shapes").toDouble();
    z_value = z_value_unhighlighted;

    int size = settings.value("map/intruder_size").toInt();

    graphics_intruder = new GraphicsIntruder(size, palette, this);
    graphics_intruder->setZValue(z_value);

    graphics_text = new GraphicsText(name, QColor(Qt::red), this);
    graphics_text->setZValue(z_value);

    setZoomFactor(1.1);

}


void IntruderItem::addToMap(MapWidget* map) {
    map->scene()->addItem(graphics_intruder);
    map->scene()->addItem(graphics_text);
}

void IntruderItem::updateGraphics(MapWidget* map) {
    QPointF scene_pos = map->scenePoint(latlon);
    graphics_intruder->setPos(scene_pos);
    double s = getScale(map->zoom(), map->scaleFactor());
    graphics_intruder->setScale(s);
    graphics_intruder->setRotation(course);

    double r = map->getRotation();
    auto rot = QTransform().rotate(-r);
    graphics_text->setPos(scene_pos + rot.map(QPointF(5*s, -30*s)));
    graphics_text->setScale(s);
    graphics_text->setRotation(-r);
}

void IntruderItem::setPosition(Point2DLatLon pt) {
    latlon = pt;
    emit itemChanged();
}

void IntruderItem::setCourse(double c) {
    course = c;
    emit itemChanged();
}

void IntruderItem::removeFromScene(MapWidget* map) {
    assert(graphics_intruder != nullptr);
    map->scene()->removeItem(graphics_intruder);
    map->scene()->removeItem(graphics_text);
    delete graphics_intruder;
    delete graphics_text;
}

void IntruderItem::setForbidHighlight(bool fh) {
    graphics_intruder->setForbidHighlight(fh);
}

void IntruderItem::setEditable(bool ed) {
    (void)ed;
}

ItemType IntruderItem::getType() {
    return ITEM_INTRUDER;
}

void IntruderItem::updateZValue() {
    graphics_intruder->setZValue(z_value);
    graphics_text->setZValue(z_value);
}
