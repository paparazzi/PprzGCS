#ifndef SMPATHITEM_H
#define SMPATHITEM_H

#include "fpeditstatemachine.h"
#include "path.h"
#include "waypointitem.h"

class SmPathItem : public FpEditStateMachine
{
public:
    SmPathItem(MapWidget* map);
    ~SmPathItem();
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QColor color, MapItem* item = nullptr);

private:

    enum State {
        IDLE,
        PRESS_INI,
        MOVE_INI,
        MOVING,
    };

    Path* path;
    WaypointItem* lastWp;
    WaypointItem* previousWp;
    State state;
    QPointF pressPos;

};

#endif // SMPATHITEM_H
