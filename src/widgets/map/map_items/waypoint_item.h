#ifndef WAYPOINTITEM_H
#define WAYPOINTITEM_H

#include "map_item.h"
#include <QBrush>
#include "graphics_point.h"
#include "waypoint.h"
#include <memory>

class WaypointItem : public MapItem
{
        Q_OBJECT
public:
    WaypointItem(Point2DLatLon pt, QString ac_id, qreal z_value, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    WaypointItem(shared_ptr<Waypoint> wp, QString ac_id, qreal z_value, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    // return position of the underlying "real" waypoint
    Point2DLatLon position() {return Point2DLatLon(original_waypoint);}
    shared_ptr<Waypoint> getOriginalWaypoint() {return original_waypoint;}
    shared_ptr<Waypoint> waypoint() {return _waypoint;}
    void setPosition(Point2DLatLon ll);
    QPointF scenePos();
    virtual void addToMap(MapWidget* map);
    virtual void setHighlighted(bool h);
    virtual void setZValue(qreal z);
    virtual void setForbidHighlight(bool fh);
    virtual void setEditable(bool ed);
    virtual void removeFromScene(MapWidget* map);
    virtual void updateGraphics(MapWidget* map);
    void updatePosition();
    virtual ItemType getType() {return ITEM_WAYPOINT;}
    void setIgnoreEvent(bool ignore);
    bool isMoving() {return moving;}
    void setStyle(GraphicsPoint::Style s){point->setStyle(s);}
    void setMoving(bool m) {moving = m;}

signals:
    void waypointMoved(Point2DLatLon latlon_pos);
    void waypointMoveFinished();
    void waypointDoubleClicked();


private:
    void init();
    QString name;
    GraphicsPoint * point;
    QGraphicsTextItem* graphics_text;
    shared_ptr<Waypoint> original_waypoint;
    // waypoint used to draw this WaypointItem
    shared_ptr<Waypoint> _waypoint;
    int altitude;
    bool highlighted;
    bool moving;
};

#endif // WAYPOINTITEM_H
