#include "smwaypointitem.h"
#include "maputils.h"
#include <QDebug>
#include <QApplication>
#include "mapwidget.h"

SmWaypointItem::SmWaypointItem(MapWidget* map) :
    FpEditStateMachine (map),
    wp(nullptr), state(IDLE)
{

}

SmWaypointItem::~SmWaypointItem(){

}

//SE_PRESS,
//SE_MOVE,
//SE_RELEASE,
//SE_DOUBLE_CLICK,

MapItem* SmWaypointItem::update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QColor color, MapItem* item) {
    (void) item;    // A waypoint can't be edited (just move it!)
    (void) waypoint;    //TODO : use the waypoint to create a waypoint at the same position for another drone ?
    Point2DLatLon latlon(0, 0);
    if(event_type == FPEE_WP_CLICKED) {
        assert(waypoint != nullptr);
    } else if(event_type == FPEE_CANCEL) {
        assert(waypoint == nullptr && mouseEvent == nullptr);
    } else {
        assert(mouseEvent != nullptr);
        latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(map->zoom()), map->tileSize());
    }

    switch (state) {
    case IDLE:
        switch (event_type) {
        case FPEE_SC_PRESS:
            if(mouseEvent->button() == Qt::LeftButton) {
                wp = new WaypointItem(latlon, 20, color, 50, map);
                state = MOVING;
                mouseEvent->accept();
                return wp;
            }
            break;
        case FPEE_SC_DOUBLE_CLICK:
            state = IDLE;
            break;
        default:
            break;
        }
        break;
    case MOVING:
        switch (event_type) {
        case FPEE_SC_MOVE:
            assert(wp != nullptr);
            wp->setPosition(latlon);
            break;
        case FPEE_SC_RELEASE:
            state = IDLE;
            break;
        default:
            break;
        }
        break;
    }
    return nullptr;
}
