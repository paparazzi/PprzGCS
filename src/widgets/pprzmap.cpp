#include "pprzmap.h"
#include "ui_pprzmap.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QMenu>
#include <QAction>
#include "mapscene.h"
#include <QGraphicsSceneMouseEvent>
#include "waypointitem.h"
#include "circleitem.h"
#include "maputils.h"
#include "path.h"
#include "smwaypointitem.h"
#include <QCursor>

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap),
    drawState(false),
    interaction_state(PMIS_OTHER), fp_edit_sm(nullptr)
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());
    connect(
       scene, &MapScene::eventScene,
        [=](FPEditEvent eventType, QGraphicsSceneMouseEvent *mouseEvent) {
        if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
            MapItem* item = fp_edit_sm->update(eventType, mouseEvent, ui->map->zoom(), Qt::green);
            (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
        }
//            if(drawState == 0) {
//                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
//                Point2DLatLon latlon2 = latlonPoint(mouseEvent->scenePos() + QPointF(100, 100), zoomLevel(ui->map->zoom()), ui->map->tileSize());
//                Point2DLatLon latlon3 = latlonPoint(mouseEvent->scenePos() + QPointF(100, -100), zoomLevel(ui->map->zoom()), ui->map->tileSize());
//                Path* s = new Path(latlon, Qt::blue, ui->map->tileSize(), ui->map->zoom(), 50);
//                s->addPoint(latlon2);
//                s->addPoint(latlon3);
//                ui->map->addItem(s);
//                items.append(s);
//            } else if(drawState == 1){
//                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
//                CircleItem* ci = new CircleItem(latlon, 100, Qt::magenta, ui->map->tileSize(), ui->map->zoom(), 50);
//                ui->map->addItem(ci);
//                items.append(ci);
//            } else {
//                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
//                WaypointItem* w = new WaypointItem(latlon, 50, Qt::red, ui->map->tileSize(), ui->map->zoom(), 50);
//                ui->map->addItem(w);
//                items.append(w);
//            }
        }
    );

}

PprzMap::~PprzMap()
{
    delete ui;
}


void PprzMap::keyPressEvent(QKeyEvent *event) {
    (void)event;
}

void PprzMap::keyReleaseEvent(QKeyEvent *event) {
    (void)event;
    if(event->key() == Qt::Key_Space) {
        interaction_state = PMIS_FLIGHT_PLAN_EDIT;
        switch (drawState) {
        case 0:
            fp_edit_sm = new SmWaypointItem(ui->map->tileSize(), ui->map);
            ui->map->setCursor(QCursor(QPixmap(":/pictures/cursor_waypoint_black.svg"),0, 0));
            break;
        case 1:
            fp_edit_sm = nullptr;
            ui->map->setCursor(QCursor(QPixmap(":/pictures/cursor_circle_black.svg"),0, 0));
            break;
        case 2:
            fp_edit_sm = nullptr;
            ui->map->setCursor(QCursor(QPixmap(":/pictures/cursor_path_black.svg"),0, 0));
            break;
        default:
            break;
        }
        drawState = (drawState + 1) % 3;
    }
    else if(event->key() == Qt::Key_Escape) {
        interaction_state = PMIS_OTHER;
        ui->map->setCursor(Qt::ArrowCursor);
        //ui->map->setPanMask(Qt::LeftButton | Qt::MiddleButton);
    }
    else if (event->key() == Qt::Key_H) {
        for(auto mp: items) {
            mp->setHighlighted(false);
        }
    }
}
