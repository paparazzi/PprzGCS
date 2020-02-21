#include "smpathitem.h"
#include "mapwidget.h"
#include <QApplication>
#include <QDebug>
#include <iostream>

SmPathItem::SmPathItem(MapWidget* map) :
    FpEditStateMachine (map),
    path(nullptr), lastWp(nullptr), previousWp(nullptr), state(IDLE)
{

}

SmPathItem::~SmPathItem() {

}

MapItem* SmPathItem::update(FPEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QColor color, MapItem* item) {
    QPointF dp;
    double d;
    WaypointItem* nextWp = nullptr;

    Point2DLatLon latlon(0, 0);
    if(event_type == FPEE_WP_CLICKED) {
        assert(waypoint != nullptr);
    } else if(event_type == FPEE_CANCEL) {
        assert(waypoint == nullptr && mouseEvent == nullptr);
    }
    else {
        assert(mouseEvent != nullptr);
        latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(map->zoom()), map->tileSize());
    }

    // if we are editing a path, go direct to MOVING state !
    if(item != nullptr) {
        if(item->getType() != ITEM_PATH) {
            std::runtime_error("Can't edit something else than paths!");
        }
        path = static_cast<Path*>(item);
        previousWp = path->getLastWaypoint();
        lastWp = new WaypointItem(latlon, 20, color, 50, map);
        path->addPoint(lastWp);
        lastWp->setIgnoreEvent(true);
        previousWp->setIgnoreEvent(false);
        map->setMouseTracking(true);
        state = MOVING;
    }

    switch (state) {
    case IDLE:
        switch (event_type) {
        case FPEE_SC_PRESS:
            if(mouseEvent->button() == Qt::LeftButton) {
                if(item != nullptr) {
                    std::runtime_error("Editing path should not be IDLE state!");
                }
                pressPos = mouseEvent->scenePos();
                previousWp = nullptr;
                lastWp = new WaypointItem(latlon, 20, color, 50, map);
                path = new Path(lastWp, color, 50, map);
                mouseEvent->accept();
                state = PRESS_INI;
                return path;
            }
            break;
        case FPEE_WP_CLICKED:
            if(item != nullptr) {
                std::runtime_error("Editing path should not be IDLE state!");
            }
            path = static_cast<Path*>(item);
            previousWp = nullptr;
            lastWp = new WaypointItem(waypoint->position(), 20, color, 50, map);
            path->addPoint(lastWp);
            path->setLastLineIgnoreEvents(true);
            lastWp->setIgnoreEvent(true);
            map->setMouseTracking(true);
            state = MOVING;
            break;
        default:
            break;
        }
        break;
    case PRESS_INI:
        switch (event_type) {
        case FPEE_SC_MOVE:
            dp = mouseEvent->scenePos()-pressPos;
            d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
            if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
                state = MOVE_INI;
            }
            mouseEvent->accept();
            break;
        case FPEE_SC_RELEASE:
            if(mouseEvent->button() == Qt::LeftButton) {
                nextWp = new WaypointItem(latlon, 20, color, 50, map);
                path->addPoint(nextWp);
                path->setLastLineIgnoreEvents(true);
                previousWp = lastWp;
                lastWp = nextWp;
                lastWp->setIgnoreEvent(true);
                previousWp->setIgnoreEvent(false);
                map->setMouseTracking(true);
                state = MOVING;
                mouseEvent->accept();
            }
            break;
        default:
            break;
        }
        break;
    case MOVE_INI:
        switch (event_type) {
        case FPEE_SC_MOVE:
            lastWp->setPosition(latlon);
            mouseEvent->accept();
            break;
        case FPEE_SC_RELEASE:
            if(mouseEvent->button() == Qt::LeftButton) {
                nextWp = new WaypointItem(latlon, 20, color, 50, map);
                path->addPoint(nextWp);
                path->setLastLineIgnoreEvents(true);
                previousWp = lastWp;
                lastWp = nextWp;
                lastWp->setIgnoreEvent(true);
                previousWp->setIgnoreEvent(false);
                map->setMouseTracking(true);
                state = MOVING;
                mouseEvent->accept();
            }
            break;
        default:
            break;
        }
        break;
    case MOVING:
        switch (event_type) {
        case FPEE_SC_PRESS:
            if(mouseEvent->button() == Qt::RightButton) {
                state = IDLE;
                lastWp->setIgnoreEvent(false);
                path->setLinesIgnoreEvents(false);
                map->setMouseTracking(false);
                mouseEvent->accept();
            }
            if(mouseEvent->button() == Qt::LeftButton) {
                mouseEvent->accept();
            }
            // other buttons are not accepted.
            //This way, you can still drag the map with the middle button.
            break;
        case FPEE_SC_MOVE:
            //move lastWp and update graphics of path
            lastWp->setPosition(latlon);
            path->updateGraphics();
            mouseEvent->accept();
            break;
        case FPEE_SC_RELEASE:
            if(mouseEvent->button() == Qt::LeftButton) {
                nextWp = new WaypointItem(latlon, 20, color, 50, map);
                path->addPoint(nextWp);
                path->setLastLineIgnoreEvents(true);
                previousWp = lastWp;
                lastWp = nextWp;
                lastWp->setIgnoreEvent(true);
                previousWp->setIgnoreEvent(false);
                mouseEvent->accept();
            }
            if(mouseEvent->button() == Qt::RightButton) {
                // finish interaction, to not sending a release to an other widget
                // that hasn't got any press.
                mouseEvent->accept();
            }
            break;
        case FPEE_SC_DOUBLE_CLICK:
            //finish it !
            mouseEvent->accept();
            break;
        case FPEE_WP_CLICKED:
            if(waypoint != previousWp) {
                //waypoint must be of the same A/C ! (but should be handled before, in PprzMap)
                //delete last waypoint and last segment
                path->removeLastWaypoint();
                path->addPoint(waypoint);
                path->addPoint(lastWp);
                previousWp = waypoint;
                path->setLastLineIgnoreEvents(true);
                lastWp->setIgnoreEvent(true);
                previousWp->setIgnoreEvent(false);
            } else {
                std::cout << "Waypoints following each other in Path must be different!" << std::endl;
            }
            break;
        case FPEE_CANCEL:
            //delete last waypoint and last segment
            path->removeLastWaypoint();         //lastWp removed
            path->setLinesIgnoreEvents(false);  //re-enable events on all lines
            map->removeItem(lastWp);    // we created lastWp, so its safe to delete it.
            state = IDLE;
            lastWp = nullptr;
            previousWp = nullptr;
            path = nullptr;
            map->setMouseTracking(false);
            map->scene()->setShortcutItems(false);
            break;
        }
        break;
    }

    return nullptr;
}
