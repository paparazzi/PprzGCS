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
#include "graphicspoint.h"
#include "segment.h"

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap), drawState(false)
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());
    connect(
       scene, &MapScene::rightClick,
        [=](QGraphicsSceneMouseEvent *mouseEvent) {
            if(drawState) {
                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                Point2DLatLon latlon2 = latlonPoint(mouseEvent->scenePos() + QPointF(100, 100), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                Segment* s = new Segment(latlon, latlon2, Qt::blue, ui->map->tileSize(), ui->map->zoom(), 15);
                ui->map->addItem(s);
            } else {
                Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
                WaypointItem* w = new WaypointItem(latlon, 50, Qt::red, ui->map->tileSize(), ui->map->zoom(), 15);
                ui->map->addItem(w);
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
        if(!drawState) {
            ui->map->setDragMode(QGraphicsView::NoDrag);
        } else {
            ui->map->setDragMode(QGraphicsView::ScrollHandDrag);
        }
        drawState = !drawState;
        qDebug() << "SPAAAAAACE!";
    }
}
