#ifndef CIRCLEITEM_H
#define CIRCLEITEM_H

#include "mapitem.h"
#include "waypointitem.h"
#include <QGraphicsEllipseItem>

class CircleItem : public MapItem
{
    Q_OBJECT
public:
    explicit CircleItem(Point2DLatLon pt, double radius, QColor color, double zoom, int tile_size, double neutral_scale_zoom = 15, QGraphicsItem *parent = nullptr);
    virtual void scaleToZoom(double zoom, double viewScale);

signals:

private:
    WaypointItem* center;
    double _radius;

QGraphicsEllipseItem* circle;

};

#endif // CIRCLEITEM_H
