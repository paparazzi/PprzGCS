#ifndef CIRCLEITEM_H
#define CIRCLEITEM_H

#include "map_item.h"
#include "waypoint_item.h"
#include "graphics_circle.h"

class MapWidget;

class CircleItem : public MapItem
{
    Q_OBJECT
public:
    explicit CircleItem(Point2DLatLon pt, double radius, QString ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    explicit CircleItem(WaypointItem* center, double radius, QString ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15);
    Point2DLatLon position() {return center->position();}
    void setPosition(Point2DLatLon ll) {center->setPosition(ll);};
    GraphicsCircle* getGraphicsCircle() {return circle;}
    virtual void setHighlighted(bool h);
    void setForbidHighlight(bool fh);
    virtual void setEditable(bool ed);
    virtual void setZValue(qreal z);
    virtual void updateGraphics();
    virtual void removeFromScene();
    virtual ItemType getType() {return ITEM_CIRCLE;}
    double radius() {return _radius;}
    ///
    /// \brief setRadius
    /// \param radius in meters
    ///
    void setRadius(double radius);

signals:
    void circleMoved(Point2DLatLon latlon_pos);
    void circleScaled(double radius);

protected:

private:
    void init(WaypointItem* center, double radius);
    WaypointItem* center;
    GraphicsCircle* circle;
    double _radius;
    int altitude;
    int stroke;

    bool highlighted;
};

#endif // CIRCLEITEM_H
