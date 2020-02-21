#ifndef CIRCLEITEM_H
#define CIRCLEITEM_H

#include "mapitem.h"
#include "waypointitem.h"
#include "graphicscircle.h"

class MapWidget;

class CircleItem : public MapItem
{
    Q_OBJECT
public:
    explicit CircleItem(Point2DLatLon pt, double radius, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    explicit CircleItem(WaypointItem* center, double radius, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15);
    Point2DLatLon position() {return center->position();}
    void setPosition(Point2DLatLon ll) {center->setPosition(ll);};
    virtual void setHighlighted(bool h);
    virtual void setZValue(qreal z);

signals:
    void circleMoved(Point2DLatLon latlon_pos);
    void circleScaled(double radius);

protected:
    virtual void updateGraphics();

private:
    void init(WaypointItem* center, double radius, QColor color, MapWidget* map);
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
