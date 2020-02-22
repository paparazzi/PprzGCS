#ifndef SEGMENT_H
#define SEGMENT_H

#include "mapitem.h"
#include <QBrush>
#include "waypointitem.h"
#include "graphicsline.h"

class Path : public MapItem
{
    Q_OBJECT
public:
    explicit Path(Point2DLatLon start, int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    explicit Path(WaypointItem* wpStart, int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    void addPoint(Point2DLatLon pos);
    void addPoint(WaypointItem* waypoint);
    virtual void setHighlighted(bool h);
    virtual void setForbidHighlight(bool sh);
    virtual void setEditable(bool ed);
    virtual void setZValue(qreal z);
    virtual void updateGraphics();
    virtual void removeFromScene();
    virtual ItemType getType() {return ITEM_PATH;}
    WaypointItem* getLastWaypoint() {return waypoints.last();}
    void setLinesIgnoreEvents(bool ignore);
    void setLastLineIgnoreEvents(bool ignore);
    void removeLastWaypoint();

signals:

public slots:

protected:


private:
    void init(WaypointItem* startWp);
    QList<WaypointItem*> waypoints;
    QList<GraphicsLine*> lines;
    int line_widht;
    bool highlighted;
    //QGraphicsLineItem* line;
};

#endif // SEGMENT_H
