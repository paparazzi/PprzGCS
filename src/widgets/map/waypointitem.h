#ifndef WAYPOINTITEM_H
#define WAYPOINTITEM_H

#include "mapitem.h"
#include <QBrush>
#include "graphicspoint.h"

class WaypointItem : public MapItem
{
        Q_OBJECT
public:
    WaypointItem(Point2DLatLon pt, int size, QColor color, int tile_size, double zoom, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    void scaleToZoom(double zoom, double viewScale);
    void add_to_scene(QGraphicsScene* scene);
    Point2DLatLon position() {return latlon;}
    void setPosition(Point2DLatLon ll);

signals:
    void waypointMoved(Point2DLatLon latlon_pos);

private:
    GraphicsPoint * point;
    Point2DLatLon latlon;
    int altitude;
    //bool movable;


//    double zoom_factor;
//    double neutral_scale_zoom;
//    int tile_size;
//    double _zoom;
};

#endif // WAYPOINTITEM_H
