#include "map2d.h"
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include "point2dlatlon.h"
#include "point2dtile.h"

#define SIZE 10000


Map2D::Map2D(QWidget *parent) : QGraphicsView(parent), numericZoom(0)
{
    scene = new QGraphicsScene(-500, -500, 524288*256, 524288*256, parent);
    //scene = new QGraphicsScene(parent);
    tileProvider.setTileSource(HIKING);
    //tileProvider.setTileSource(OSM_CLASSIC);
    setScene(scene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setBackgroundBrush(QBrush(Qt::darkGreen));

    connect(DispatcherUi::get(), SIGNAL(ac_selected(int)), this, SLOT(acChanged(int)));

    connect(&tileProvider, SIGNAL(tileReady(TileItem*, Point2DTile)), this, SLOT(handleTile(TileItem*, Point2DTile)));

    setPos(Point2DLatLon(43.4625, 1.2732, 6));
}


void Map2D::wheelEvent(QWheelEvent* event) {
    setTransformationAnchor(QGraphicsView::NoAnchor);

    if( (numericZoom != 0) ||
        (event->delta() > 0 && tileProvider.zoomLevel() == tileProvider.ZOOM_MAX)) {
    // "numeric" zoom

        double scaleIni = 1 + NUMERIC_ZOOM_FACTOR*numericZoom;

        if(event->delta() > 0) {
            numericZoom += 1;
        } else {
            numericZoom -= 1;
        }
        double scaleTarget = 1 + NUMERIC_ZOOM_FACTOR*numericZoom;

        double scaleFactor = scaleTarget/scaleIni;

        QPointF oldPos = mapToScene(event->pos());

        scale(scaleFactor, scaleFactor);

        QPointF newPos = mapToScene(event->pos());
        QPointF delta = newPos - oldPos;
        translate(delta.x(), delta.y());

    }
    else {
        QPointF oldPos = mapToScene(event->pos());
        int dx = event->pos().x() - width()/2;
        int dy = event->pos().y() - height()/2;

        double xEvent = oldPos.x()/tileProvider.TILE_SIZE;
        double yEvent = oldPos.y()/tileProvider.TILE_SIZE;

        Point2DLatLon latLon(Point2DTile(xEvent, yEvent, tileProvider.zoomLevel()));

        if(event->delta() > 0) {
            latLon.setZoom(latLon.zoom() + 1);
        } else {
            latLon.setZoom(latLon.zoom() - 1);
        }

        Point2DTile tt(latLon);
        double xx = tt.x()*tileProvider.TILE_SIZE - dx;
        double yy = tt.y()*tileProvider.TILE_SIZE - dy;

        setPos(latLon, xx, yy);
    }
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
            Point2DTile coor(x, y, tileProvider.zoomLevel());
            tileProvider.fetch_tile(coor);
        }
    }
}

void Map2D::acChanged(int ac_id) {
    (void)ac_id;
    setPos(Point2DLatLon(45.5, 1.34, 16));
}

void Map2D::handleTile(TileItem* tile, Point2DTile coorI) {
    if(!tile->isInScene()) {    // Not in scene, so lets add it
        scene->addItem(tile);
        tile->setInScene(true);
    }
    if(!tile->isVisible()) {    // in scene but hidden, lets show it. TODO: what if this slot is called just atfer a zoom change ?
        if(coorI.zoom() == tileProvider.zoomLevel()) {
            tile->show();
        }
    }

    QPointF pos = QPointF(
        tileProvider.TILE_SIZE*(coorI.x()),
        tileProvider.TILE_SIZE*(coorI.y())
    );
    tile->setPos(pos);
}

void Map2D::setPos(Point2DLatLon latLon, double cx, double cy) {
    Point2DTile coorD(latLon);

    tileProvider.setZoomLevel(coorD.zoom());

    int xMin = coorD.xi() - width()/tileProvider.TILE_SIZE - 2;
    int xMax = coorD.xi() + width()/tileProvider.TILE_SIZE + 2;
    int yMin = coorD.yi() - height()/tileProvider.TILE_SIZE - 2;
    int yMax = coorD.yi() + height()/tileProvider.TILE_SIZE + 2;

    for(int x=xMin; x<=xMax; x++) {
        for(int y=yMin; y<=yMax; y++) {
            Point2DTile coor(x, y, tileProvider.zoomLevel());
            tileProvider.fetch_tile(coor);
        }
    }

    if(cx==0 && cy==0) {
        cx = tileProvider.TILE_SIZE*coorD.x();
        cy = tileProvider.TILE_SIZE*coorD.y();
    }

    centerOn(
        QPointF(
         cx,
         cy
     ));
}
