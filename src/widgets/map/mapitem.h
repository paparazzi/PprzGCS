#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include "point2dlatlon.h"

class MapItem : public QGraphicsItemGroup
{
public:
    MapItem(Point2DLatLon pt, QGraphicsItem *parent = nullptr);
    MapItem(QGraphicsItem* child, Point2DLatLon pt, double neutral_scale_zoom = 15, QGraphicsItem *parent = nullptr);
    double zoomFactor() {return zoom_factor;}
    double neutralScaleZoom() {return neutral_scale_zoom;}
    void setZoomFactor(double zf) {zoom_factor = zf;}

    void scaleToZoom(double zoom, double viewScale);


    Point2DLatLon position() {return latlon;}

private:
    Point2DLatLon latlon;
    double zoom_factor;
    double neutral_scale_zoom;
};

#endif // MAPITEM_H
