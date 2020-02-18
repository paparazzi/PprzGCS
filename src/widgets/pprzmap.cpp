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

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap), drawState(false)
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());
    connect(
       scene, &MapScene::rightClick,
        [=](QGraphicsSceneMouseEvent *mouseEvent) {
        int size = 30;
        QGraphicsEllipseItem *circle = new QGraphicsEllipseItem(- size/2,- size/2, size, size);
        circle->setBrush(QBrush(Qt::blue));
        Point2DLatLon latlon = ui->map->latlonPoint(mouseEvent->scenePos(), ui->map->zoomLevel());
        ui->map->addItem(circle, latlon, 10, 1.15);
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
