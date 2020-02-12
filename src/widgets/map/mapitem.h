#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include "point2dlatlon.h"

class MapItem : public QGraphicsItemGroup
{
public:
    MapItem(Point2DLatLon pt, QGraphicsItem *parent = nullptr);
    MapItem(QGraphicsItem* child, Point2DLatLon pt, QGraphicsItem *parent = nullptr);

    Point2DLatLon position() {return latlon;}

private:
    Point2DLatLon latlon;
};

#endif // MAPITEM_H
