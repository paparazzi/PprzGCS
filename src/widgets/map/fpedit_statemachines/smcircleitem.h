#ifndef SMCIRCLEITEM_H
#define SMCIRCLEITEM_H

#include "fpeditstatemachine.h"
#include "circleitem.h"

enum FPEditCircleSMState {
    FPECSMS_IDLE,
    FPECSMS_PRESSED,   // mouse pressed, move < hysteresis.
    FPECSMS_DRAGING,   // mouse dragged after the press. expand the circle according to mouse moves
    FPECSMS_RELEASED,  // mouse released after the press. Wait for next press event to set circle radius.
};

class SmCircleItem : public FpEditStateMachine
{
public:
    SmCircleItem(MapWidget* map);
    ~SmCircleItem();
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QColor color, MapItem* item = nullptr);

private:
    void adjustCircleRadius(QGraphicsSceneMouseEvent* mouseEvent);
    QPointF pressPos;
    CircleItem* cir;
    FPEditCircleSMState state;
};

#endif // SMCIRCLEITEM_H
