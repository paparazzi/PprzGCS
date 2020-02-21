#ifndef SMCIRCLEITEM_H
#define SMCIRCLEITEM_H

#include "fpeditstatemachine.h"
#include "circleitem.h"

class SmCircleItem : public FpEditStateMachine
{
public:
    SmCircleItem(MapWidget* map);
    ~SmCircleItem();
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QColor color, MapItem* item = nullptr);

private:
    enum State {
        IDLE,
        PRESSED,   // mouse pressed, move < hysteresis.
        DRAGING,   // mouse dragged after the press. expand the circle according to mouse moves
        RELEASED,  // mouse released after the press. Wait for next press event to set circle radius.
    };

    void adjustCircleRadius(QGraphicsSceneMouseEvent* mouseEvent);
    QPointF pressPos;
    CircleItem* cir;
    State state;
};

#endif // SMCIRCLEITEM_H
