#ifndef WAYPOINTITEM_H
#define WAYPOINTITEM_H

#include "mapitem.h"
#include <QBrush>
#include "graphicspoint.h"

class WaypointItem : public MapItem
{
        Q_OBJECT
public:
    WaypointItem(Point2DLatLon pt, int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    Point2DLatLon position() {return latlon;}
    void setPosition(Point2DLatLon ll);
    QPointF scenePos();
    virtual void setHighlighted(bool h);
    virtual void setZValue(qreal z);
    virtual void setForbidHighlight(bool fh);
    virtual void setEditable(bool ed);
    virtual void removeFromScene();
    virtual ItemType getType() {return ITEM_WAYPOINT;}
    void setIgnoreEvent(bool ignore);

signals:
    void waypointMoved(Point2DLatLon latlon_pos);

protected:
    virtual void updateGraphics();

private:
    GraphicsPoint * point;
    Point2DLatLon latlon;
    int altitude;
    bool highlighted;
};

#endif // WAYPOINTITEM_H
