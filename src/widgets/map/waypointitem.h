#ifndef WAYPOINTITEM_H
#define WAYPOINTITEM_H

#include "mapitem.h"
#include <QBrush>
#include <QGraphicsEllipseItem>

class WaypointItem : public MapItem
{
        Q_OBJECT
public:
    WaypointItem(Point2DLatLon pt, int size, QColor color, int tile_size, double neutral_scale_zoom = 15, QGraphicsItem *parent = nullptr);
    void scaleToZoom(double zoom, double viewScale);
    Point2DLatLon position() {return latlon;}
    void setPosition(Point2DLatLon ll) {latlon = ll;}

signals:
    void waypointMoved(Point2DLatLon scenePos);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QGraphicsEllipseItem * ellipse;

    QPointF pressPos;
    MoveState move_state;
    bool movable;

    Point2DLatLon latlon;
//    double zoom_factor;
//    double neutral_scale_zoom;
//    int tile_size;
//    double _zoom;
};

#endif // WAYPOINTITEM_H
