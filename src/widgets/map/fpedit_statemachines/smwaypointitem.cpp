#include "smwaypointitem.h"
#include "maputils.h"
#include <QDebug>
#include <QApplication>
#include "mapwidget.h"

SmWaypointItem::SmWaypointItem(int tile_size, MapWidget* map) :
    FpEditStateMachine (tile_size, map),
    wp(nullptr), state(FPEWSMS_IDLE)
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
    } else {
        assert(mouseEvent != nullptr);
        latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(map->zoom()), tile_size);
    }

    switch (state) {
    case FPEWSMS_IDLE:
        switch (event_type) {
        case FPEE_SC_PRESS:
            if(mouseEvent->button() == Qt::LeftButton) {
                wp = new WaypointItem(latlon, 20, color, 50, map);
                state = FPEWSMS_MOVING;
                mouseEvent->accept();
                return wp;
            }
            break;
        case FPEE_SC_DOUBLE_CLICK:
            state = FPEWSMS_IDLE;
            break;
        default:
            break;
        }
        break;
    case FPEWSMS_MOVING:
        switch (event_type) {
        case FPEE_SC_MOVE:
            assert(wp != nullptr);
            wp->setPosition(latlon);
            break;
        case FPEE_SC_RELEASE:
            state = FPEWSMS_IDLE;
            break;
        default:
            break;
        }
        break;
    }
    return nullptr;
}
