#ifndef SMWAYPOINTITEM_H
#define SMWAYPOINTITEM_H

#include "fpeditstatemachine.h"
#include "waypointitem.h"

class SmWaypointItem : public FpEditStateMachine
{
public:
    SmWaypointItem(MapWidget* map);
    ~SmWaypointItem();
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, int ac_id, MapItem* item = nullptr);

private:

    enum State {
        IDLE,
        MOVING,
    };

    WaypointItem* wp;
    State state;
};

#endif // SMWAYPOINTITEM_H
