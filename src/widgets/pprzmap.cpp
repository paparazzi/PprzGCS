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

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap), drawState(false)
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());
    connect(
       scene, &MapScene::rightClick,
        [=](QGraphicsSceneMouseEvent *mouseEvent) {
            if(drawState == 0) {
                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                Point2DLatLon latlon2 = latlonPoint(mouseEvent->scenePos() + QPointF(100, 100), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                Point2DLatLon latlon3 = latlonPoint(mouseEvent->scenePos() + QPointF(100, -100), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                Path* s = new Path(latlon, Qt::blue, ui->map->tileSize(), ui->map->zoom(), 15);
                s->addPoint(latlon2);
                s->addPoint(latlon3);
                ui->map->addItem(s);
                items.append(s);
            } else if(drawState == 1){
                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
//                WaypointItem* w = new WaypointItem(latlon, 50, Qt::red, ui->map->tileSize(), ui->map->zoom(), 15);
//                ui->map->addItem(w);
                CircleItem* ci = new CircleItem(latlon, 100, Qt::magenta, ui->map->zoom(), ui->map->tileSize());
                ui->map->addItem(ci);
                items.append(ci);
            } else {
                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                WaypointItem* w = new WaypointItem(latlon, 50, Qt::red, ui->map->tileSize(), ui->map->zoom(), 15);
                ui->map->addItem(w);
                items.append(w);
            }
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
        drawState = (drawState + 1) % 3;
        qDebug() << "SPAAAAAACE!";
    }
    else if (event->key() == Qt::Key_H) {
        for(auto mp: items) {
            mp->setHighlighted(false);
        }
    }
}
