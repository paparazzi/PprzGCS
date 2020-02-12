#include "mapitem.h"
#include "math.h"

MapItem::MapItem(Point2DLatLon pt, QGraphicsItem *parent) : QGraphicsItemGroup(parent),
    latlon(pt), zoom_factor(1), neutral_scale_zoom(15)
{
}

MapItem::MapItem(QGraphicsItem* child, Point2DLatLon pt, double neutral_scale_zoom, QGraphicsItem *parent) : QGraphicsItemGroup(parent),
    latlon(pt), zoom_factor(1), neutral_scale_zoom(neutral_scale_zoom)
{
    addToGroup(child);
}

void MapItem::scaleToZoom(double zoom, double viewScale) {
    double s = pow(zoom_factor, zoom - neutral_scale_zoom)/viewScale;
    setScale(s);
}
