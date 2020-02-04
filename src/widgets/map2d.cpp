#include "map2d.h"
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>

#define SIZE 10000

/*
 * Fake map just for illustration.
 *
 *
 */

Map2D::Map2D(QWidget *parent) : QGraphicsView(parent)
{
    scene = new QGraphicsScene(-SIZE, -SIZE, 2*SIZE, 2*SIZE, parent);
    setScene(scene);
    current_ac = new QGraphicsTextItem("AC : None");
    current_ac->setScale(4);
    scene->addItem(current_ac);
    current_ac->setPos(0, -100);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setBackgroundBrush(QBrush(Qt::darkGreen));

    connect(DispatcherUi::get(), SIGNAL(ac_selected(int)), this, SLOT(acChanged(int)));

    connect(&tileProvider, SIGNAL(tileReady(TileItem*, TileCoorI)), this, SLOT(handleTile(TileItem*, TileCoorI)));

    lat0 = 43.5;
    lon0 = 1.2;

    TileCoorD coorD = tileProvider.tileCoorFromLatlon(lat0, lon0, 18);
    pos0 = QPointF(std::get<0>(coorD), std::get<1>(coorD));

    // fetch 1 tile to test
    TileCoorI coorI = COOR_I_OF_D(coorD);
    tileProvider.fetch_tile(coorI);

    TileCoorI c2 = std::make_tuple(std::get<0>(coorI) -3, std::get<1>(coorI), std::get<2>(coorI));
    tileProvider.fetch_tile(c2);
}


void Map2D::wheelEvent(QWheelEvent* event) {
    setTransformationAnchor(QGraphicsView::NoAnchor);
    qreal zoomFactor = 1.25;
    if(event->delta() < 0) {
        zoomFactor = 1/zoomFactor;
    }

    QPointF oldPos = mapToScene(event->pos());

    scale(zoomFactor, zoomFactor);

    QPointF newPos = mapToScene(event->pos());
    QPointF delta = newPos - oldPos;
    translate(delta.x(), delta.y());
}

void Map2D::acChanged(int ac_id) {
    current_ac->setPlainText("AC : " + QString::number(ac_id));
}

void Map2D::handleTile(TileItem* tile, TileCoorI coorI) {
    QPointF pos = QPointF(
        tileProvider.TILE_SIZE*(std::get<0>(coorI) - pos0.x()),
        tileProvider.TILE_SIZE*(std::get<1>(coorI) - pos0.y())
    );
    scene->addItem(tile);
    tile->setPos(pos);
    tile->setDisplayed(true);
}

