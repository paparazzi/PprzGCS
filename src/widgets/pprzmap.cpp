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

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap), drawState(false)
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());
    connect(
       scene, &MapScene::rightClick,
        [=](QGraphicsSceneMouseEvent *mouseEvent) {
        int size = 50;

        Point2DLatLon latlon = latlonPoint(mouseEvent->scenePos(), zoomLevel(ui->map->zoom()), ui->map->tileSize());
        //WaypointItem* map_item = new WaypointItem(latlon, size, Qt::blue,  ui->map->tileSize());
        CircleItem* map_item = new CircleItem(latlon, size, Qt::blue, ui->map->zoom(), ui->map->tileSize());
        map_item->setZoomFactor(1);
        ui->map->addItem(map_item, latlon, 10);
            qDebug() << mouseEvent->scenePos();
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
