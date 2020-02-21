#ifndef FPEDITSTATEMACHINE_H
#define FPEDITSTATEMACHINE_H

#include "mapitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QColor>

class MapWidget;
class WaypointItem;

enum FPEditEvent {
    FPEE_SC_PRESS,
    FPEE_SC_MOVE,
    FPEE_SC_RELEASE,
    FPEE_SC_DOUBLE_CLICK,
    FPEE_WP_CLICKED,
};

class FpEditStateMachine
{
public:
    FpEditStateMachine(int tile_size, MapWidget* map);
    virtual ~FpEditStateMachine();
    ///
    /// \brief update
    /// \param event_type
    /// \param mouseEvent: mouse event if applicable. Must point to valid data if @event_type is FPEE_SC_*. (can be null otherwise)
    /// \param waypoint: base waypoint for the item (eg. center of a circle, waypoint in a path...). Must point to valid data if @event_type is FPEE_WP_CLICKED. (can be null otherwise)
    /// \param color
    /// \param item: item to edit (only applicable for the path?)
    /// \return
    ///
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QColor color, MapItem* item = nullptr) = 0;

protected:
    int tile_size;
    MapWidget* map;
};

#endif // FPEDITSTATEMACHINE_H
