#include "circleitem_sm.h"
#include "mapwidget.h"
#include <QApplication>
#include <QDebug>
#include <iostream>

SmCircleItem::SmCircleItem(MapWidget* map) :
        ItemEditStateMachine (map),
        cir(nullptr), state(IDLE)
{

}

SmCircleItem::~SmCircleItem() {

}

MapItem* SmCircleItem::update(SmEditEvent event_type, QGraphicsSceneMouseEvent* mouseEvent, WaypointItem* waypoint, QString ac_id, MapItem* item) {
    (void)item;
    Point2DLatLon latlon(0, 0);
    if(event_type == FPEE_WP_CLICKED) {
        assert(waypoint != nullptr);
    } else if(event_type == FPEE_CANCEL) {
        assert(waypoint == nullptr && mouseEvent == nullptr);
    } else {
        assert(mouseEvent != nullptr);
        latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(map->zoom()), map->tileSize());
    }

    // some variables used later, cause switch are so prehistoric...
    QPointF dp;
    QPointF pos;
    double d;
    WaypointItem* wp;

    switch (state) {
    case IDLE:
        switch (event_type) {
        case FPEE_SC_PRESS:
            if(mouseEvent->button() == Qt::LeftButton) {
                pressPos = mouseEvent->scenePos();
                cir = new CircleItem(latlon, 0, ac_id, 50, map);
                cir->getGraphicsCircle()->displayRadius(true);
                mouseEvent->accept();
                state = PRESSED;
                // set moving color ?
                return cir;
            }
            break;
        case FPEE_WP_CLICKED:
            if(waypoint->acId() == ac_id) {
                wp = waypoint;
            } else {
                wp = new WaypointItem(waypoint->position(), ac_id, 50, map);
            }
            cir = new CircleItem(wp, 0, ac_id, 50, map);
            cir->getGraphicsCircle()->displayRadius(true);
            map->setMouseTracking(true);
            map->scene()->setShortcutItems(true);
            state = RELEASED;
            return cir;
        default:
            break;
        }
        break;
    case PRESSED:
        switch (event_type) {
        case FPEE_SC_MOVE:
            dp = mouseEvent->scenePos()-pressPos;
            d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
            if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
                state = DRAGING;
            }
            mouseEvent->accept();
            break;
        case FPEE_SC_RELEASE:
            if(mouseEvent->button() == Qt::LeftButton) {
                map->setMouseTracking(true);
                map->scene()->setShortcutItems(true);
                state = RELEASED;
                mouseEvent->accept();
            }
            break;
        default:
            break;
        }
        break;
    case DRAGING:
        switch (event_type) {
        case FPEE_SC_MOVE:
            assert(cir != nullptr);
            adjustCircleRadius(mouseEvent);
            pos = mouseEvent->scenePos() - cir->getGraphicsCircle()->pos();
            cir->getGraphicsCircle()->setTextPos(pos);
            mouseEvent->accept();
            break;
        case FPEE_SC_RELEASE:
            if(mouseEvent->button() == Qt::LeftButton) {
                if (cir->radius() < qApp->property("CIRCLE_CREATE_MIN_RADIUS").toDouble()) {
                    std::cout
                            << "can't create circle with radius < CIRCLE_CREATE_MIN_RADIUS. Please change this parameter."
                            << std::endl;
                    map->removeItem(cir);
                } else {
                    // back to normal color ?
                    cir->getGraphicsCircle()->displayRadius(false);
                }

                state = IDLE;
            }
            break;
        default:
            break;
        }
        break;
    case RELEASED:
        switch (event_type) {
        case FPEE_SC_PRESS:
            if(mouseEvent->button() == Qt::LeftButton) {
                map->setMouseTracking(false);
                map->scene()->setShortcutItems(false);
                state = DRAGING;
                mouseEvent->accept();
            }
            break;
        case FPEE_SC_MOVE:
            adjustCircleRadius(mouseEvent);
            pos = mouseEvent->scenePos() - cir->getGraphicsCircle()->pos();
            cir->getGraphicsCircle()->setTextPos(pos);
            mouseEvent->accept();
            break;
        default:
            break;
        }
        break;
    }

    return nullptr;
}


void SmCircleItem::adjustCircleRadius(QGraphicsSceneMouseEvent* mouseEvent) {
    // TODO fix this crazy stuff here : convert latlon to scene, scenePos to meters, then back to scenePos...
    // sol1: get latlon of mouse, distance in meters with center
    // sol2: pass directly distance in scenePos to child circle, then update th radius somehow.
    QPointF center = scenePoint(cir->position(), zoomLevel(map->zoom()), map->tileSize());
    Point2DTile center_tile = Point2DTile(cir->position(), zoomLevel(map->zoom()));
    QPointF dp = mouseEvent->scenePos()- center;
    double d_tile = sqrt(dp.x()*dp.x() + dp.y()*dp.y()) / map->tileSize();
    double d_meters = distTile2Meters(center_tile.y(), d_tile, zoomLevel(map->zoom()));
    //qDebug() << "Circle state machine: moved: " << d << "   " << d2;
    cir->setRadius(d_meters);
    cir->getGraphicsCircle()->setText(QString::number(static_cast<int>(d_meters)) + "m");
}
