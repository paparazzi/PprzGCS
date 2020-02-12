#include "mapitem.h"

MapItem::MapItem(Point2DLatLon pt, QGraphicsItem *parent) : QGraphicsItemGroup(parent), latlon(pt)
{
}

MapItem::MapItem(QGraphicsItem* child, Point2DLatLon pt, QGraphicsItem *parent) : QGraphicsItemGroup(parent), latlon(pt)
{
    addToGroup(child);
}
