#ifndef SMWAYPOINTITEM_H
#define SMWAYPOINTITEM_H

#include "fpeditstatemachine.h"
#include "waypointitem.h"

enum FPEditWaypointSMState {
    FPEWSMS_IDLE,
    FPEWSMS_MOVING,
};

class SmWaypointItem : public FpEditStateMachine
{
public:
    SmWaypointItem(int tile_size, MapWidget* map);
    ~SmWaypointItem();
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, double zoom, QColor color, MapItem* item = nullptr);

private:
    WaypointItem* wp;
    FPEditWaypointSMState state;
};

#endif // SMWAYPOINTITEM_H
