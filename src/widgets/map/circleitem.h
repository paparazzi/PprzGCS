#ifndef CIRCLEITEM_H
#define CIRCLEITEM_H

#include "mapitem.h"
#include "waypointitem.h"
#include "graphicscircle.h"

class CircleItem : public MapItem
{
    Q_OBJECT
public:
    explicit CircleItem(Point2DLatLon pt, double radius, QColor color, double zoom, int tile_size, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    virtual void scaleToZoom(double zoom, double viewScale);
    void add_to_scene(QGraphicsScene* scene);
    Point2DLatLon position() {return center->position();}
    void setPosition(Point2DLatLon ll) {center->setPosition(ll);};
    virtual void setHighlighted(bool h);

signals:
    void circleMoved(Point2DLatLon latlon_pos);
    void circleScaled(double radius);

private:
    WaypointItem* center;
    GraphicsCircle* circle;
    double _radius;
    int altitude;
    int stroke;

    bool highlighted;
    //bool scalable
    //bool movable
};

#endif // CIRCLEITEM_H
