#ifndef SEGMENT_H
#define SEGMENT_H

#include "map_item.h"
#include <QBrush>
#include "waypoint_item.h"
#include "graphics_line.h"

class PathItem : public MapItem
{
    Q_OBJECT
public:
    explicit PathItem(QString ac_id, QColor color=QColor(), double neutral_scale_zoom = 15, QObject *parent = nullptr);
    void addPoint(WaypointItem* waypoint, bool own=false);
    void setClosedPath(bool closed);
    virtual void setHighlighted(bool h);
    virtual void setForbidHighlight(bool sh);
    virtual void setEditable(bool ed);
    virtual void updateZValue();
    virtual void updateGraphics(MapWidget* map);
    virtual void removeFromScene(MapWidget* map);
    virtual void addToMap(MapWidget* mw);
    virtual ItemType getType() {return ITEM_PATH;}
    WaypointItem* getLastWaypoint() {return waypoints.last();}
    QList<WaypointItem*> getWaypoints() {return waypoints;}
//    void setLinesIgnoreEvents(bool ignore);
//    void setLastLineIgnoreEvents(bool ignore);
    void removeLastWaypoint();

    void setStyle(GraphicsLine::Style s);

signals:

public slots:

protected:


private:
    QList<WaypointItem*> waypoints;
    QMap<WaypointItem*, bool> owned;
    QList<GraphicsLine*> lines;
    QList<QGraphicsItem*> to_be_added;
    QList<QGraphicsItem*> to_be_removed;
    QList<WaypointItem*> waypoints_to_remove;
    GraphicsLine* closing_line;
    int line_width;
    QColor color;
    QList<QColor> color_variants;
};

#endif // SEGMENT_H
