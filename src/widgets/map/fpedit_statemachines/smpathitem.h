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
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, int ac_id, MapItem* item = nullptr);

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

    int ac_id;

};

#endif // SMPATHITEM_H
