#include "map2d.h"
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>

#define SIZE 10000


Map2D::Map2D(QWidget *parent) : QGraphicsView(parent)
{
    //scene = new QGraphicsScene(0, 0, 524288*256, 524288*256, parent);
    scene = new QGraphicsScene(parent);
    setScene(scene);
    current_ac = new QGraphicsTextItem("AC : None");
    current_ac->setScale(4);
    scene->addItem(current_ac);
    current_ac->setPos(0, -100);
    current_ac->setZValue(10);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setBackgroundBrush(QBrush(Qt::darkGreen));

    connect(DispatcherUi::get(), SIGNAL(ac_selected(int)), this, SLOT(acChanged(int)));

    connect(&tileProvider, SIGNAL(tileReady(TileItem*, TileCoorI)), this, SLOT(handleTile(TileItem*, TileCoorI)));

    setPos(43.5, 1.2, 16);
}


void Map2D::wheelEvent(QWheelEvent* event) {
    setTransformationAnchor(QGraphicsView::NoAnchor);

    QPointF oldPos = mapToScene(event->pos());
    int dx = event->pos().x() - width()/2;
    int dy = event->pos().y() - height()/2;

    double xEvent = oldPos.x()/tileProvider.TILE_SIZE;
    double yEvent = oldPos.y()/tileProvider.TILE_SIZE;

    std::tuple<double, double> latLon = tileProvider.LatlonFromTile(xEvent, yEvent, tileProvider.zoomLevel());
    double lat = std::get<0>(latLon);
    double lon = std::get<1>(latLon);

    if(event->delta() > 0) {
        setPos(lat, lon, tileProvider.zoomLevel() + 1);
    } else {
        setPos(lat, lon, tileProvider.zoomLevel() - 1);
    }
    translate(dx, dy);
    updateTiles();
}

void Map2D::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    if(event->buttons() & Qt::LeftButton) {
        updateTiles();
    }
}

void Map2D::updateTiles() {
    QPointF topLeft = mapToScene(QPoint(0,0));
    QPointF bottomRight = mapToScene(QPoint(width(),height()));

    int xMin = static_cast<int>(topLeft.x()/tileProvider.TILE_SIZE) - 2;
    int yMin = static_cast<int>(topLeft.y()/tileProvider.TILE_SIZE) - 2;
    int xMax = static_cast<int>(bottomRight.x()/tileProvider.TILE_SIZE) + 2;
    int yMax = static_cast<int>(bottomRight.y()/tileProvider.TILE_SIZE) + 2;

    std::cout << std::to_string(xMin) << "<X<" << std::to_string(xMax) << "  " << std::to_string(yMin) << "<Y<" << std::to_string(yMax) << std::endl;

    for(int x=xMin; x<xMax; x++) {
        for(int y=yMin; y<yMax; y++) {
            TileCoorI coor = std::make_tuple(x, y, tileProvider.zoomLevel());
            tileProvider.fetch_tile(coor);
        }
    }
}

void Map2D::acChanged(int ac_id) {
    current_ac->setPlainText("AC : " + QString::number(ac_id));
    setPos(45.5, 1.34, 16);
}

void Map2D::handleTile(TileItem* tile, TileCoorI coorI) {
    if(!tile->isInScene()) {    // Not in scene, so lets add it
        scene->addItem(tile);
        tile->setInScene(true);
    }
    if(!tile->isVisible()) {    // in scene but hidden, lets show it. TODO: what if this slot is called just atfer a zoom change ?
        if(std::get<2>(tile->coordinates()) == tileProvider.zoomLevel()) {
            tile->show();
        }
    }

    QPointF pos = QPointF(
        tileProvider.TILE_SIZE*(std::get<0>(coorI)),
        tileProvider.TILE_SIZE*(std::get<1>(coorI))
    );
    tile->setPos(pos);
}

void Map2D::setPos(double lat, double lon, int z, int decX, int decY) {
    tileProvider.setZoomLevel(z);
    TileCoorD coorD = tileProvider.tileCoorFromLatlon(lat, lon, tileProvider.zoomLevel());
    double x0 = std::get<0>(coorD);
    double y0 = std::get<1>(coorD);

    double xMin = x0 - width()/tileProvider.TILE_SIZE - 2;
    double xMax = x0 + width()/tileProvider.TILE_SIZE + 2;
    double yMin = y0 - height()/tileProvider.TILE_SIZE - 2;
    double yMax = y0 + height()/tileProvider.TILE_SIZE + 2;

    for(int x=xMin; x<xMax; x++) {
        for(int y=yMin; y<yMax; y++) {
            TileCoorI coor = std::make_tuple(x, y, tileProvider.zoomLevel());
            tileProvider.fetch_tile(coor);
        }
    }

    centerOn(
        QPointF(
         tileProvider.TILE_SIZE*x0,
         tileProvider.TILE_SIZE*y0
     ));
}
