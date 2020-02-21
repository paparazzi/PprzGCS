#ifndef FPEDITSTATEMACHINE_H
#define FPEDITSTATEMACHINE_H

#include "mapitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QColor>

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
    FpEditStateMachine(int tile_size);
    virtual ~FpEditStateMachine();
    ///
    /// \brief update
    /// \param event_type
    /// \param mouseEvent
    /// \param zoom
    /// \param color
    /// \param item: item to edit (only applicable for the path?)
    /// \return
    ///
    virtual MapItem* update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, double zoom, QColor color, MapItem* item = nullptr) = 0;

protected:
    int tile_size;
};

#endif // FPEDITSTATEMACHINE_H
