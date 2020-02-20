#include "smwaypointitem.h"
#include "maputils.h"
#include <QDebug>

SmWaypointItem::SmWaypointItem(int tile_size) :
    FpEditStateMachine (tile_size),
    wp(nullptr), state(FPEWSMS_IDLE)
{

}

SmWaypointItem::~SmWaypointItem(){

}

//SE_PRESS,
//SE_MOVE,
//SE_RELEASE,
//SE_DOUBLE_CLICK,

MapItem* SmWaypointItem::update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, double zoom, QColor color) {
    (void)event_type;
    (void)mouseEvent;
    Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(zoom), tile_size);

    switch (state) {
    case FPEWSMS_IDLE:
        switch (event_type) {
        case FPEE_SC_PRESS:
            wp = new WaypointItem(latlon, 20, color, tile_size, zoom, 50);
            state = FPEWSMS_MOVING;
            return wp;
            break;
        case FPEE_SC_DOUBLE_CLICK:
            wp = new WaypointItem(latlon, 20, color, tile_size, zoom, 50);
            state = FPEWSMS_IDLE;
            //finished !
            return wp;
            break;
        case FPEE_SC_MOVE:
            // ignore move
            break;
        case FPEE_SC_RELEASE:
            throw std::runtime_error("SM Waypoint: got a release in FPEWSMS_IDLE state!");
            break;
        case FPEE_WP_CLICKED:
            break;
        }
        break;
    case FPEWSMS_MOVING:
        switch (event_type) {
        case FPEE_SC_PRESS:
        case FPEE_SC_DOUBLE_CLICK:
            throw std::runtime_error("SM Waypoint: got a press or double click in FPEWSMS_MOVING state!");
            break;
        case FPEE_SC_MOVE:
            assert(wp != nullptr);
            wp->setPosition(latlon);
            break;
        case FPEE_SC_RELEASE:
            state = FPEWSMS_IDLE;
            // finished !
            break;
        case FPEE_WP_CLICKED:
            break;
        }
        break;
    }
    return nullptr;
}
