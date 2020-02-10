#include "map2d.h"
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include "point2dlatlon.h"
#include "point2dtile.h"
#include <QtXml>
#include <QFile>

Map2D::Map2D(QWidget *parent) : QGraphicsView(parent), numericZoom(0.0), zoom(2)
{
    sourceConfigs = loadConfig("://tile_sources.xml");
    auto& config = sourceConfigs[QString("Google")];
    config->printConfig();
    tileSize = config->tileSize;

    scene = new QGraphicsScene(-500, -500, 524288*256, 524288*256, parent);
    setScene(scene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setBackgroundBrush(QBrush(Qt::darkGreen));

    tileProvider = new TileProvider(config, 0, tileSize, this);

    connect(DispatcherUi::get(), SIGNAL(ac_selected(int)), this, SLOT(acChanged(int)));

    connect(tileProvider, SIGNAL(displayTile(TileItem*, TileItem*)), this, SLOT(handleTile(TileItem*, TileItem*)));

    setPos(Point2DLatLon(43.4625, 1.2732, static_cast<int>(zoom)));
}


std::map<QString, std::unique_ptr<TileProviderConfig>> Map2D::loadConfig(QString filename) {
    std::map<QString, std::unique_ptr<TileProviderConfig>> map;

    QDomDocument xmlLayout;
    QFile f(filename);
    if(!f.open(QIODevice::ReadOnly)) {
        std::cout << "Error reading file " << filename.toStdString() << "!" << std::endl;
    }
    xmlLayout.setContent(&f);
    f.close();

    QDomElement root = xmlLayout.documentElement();
    QString rootTag = root.tagName();
    if (rootTag != "sources") {
        std::cout << "Root tag expected to be \"sources\". Is this a tileSource file ?" << std::endl;
    }

    for(int i=0; i<root.childNodes().length(); i++) {
        if(root.childNodes().item(i).isElement()) {
            QDomElement ele = root.childNodes().item(i).toElement();

            QString name = ele.attribute("name");
            map[name] = TileProviderConfig::builder{}.
                setName(name).
                setDir(ele.attribute("dir")).
                setAddr(ele.attribute("addr")).
                setPosZoom(ele.attribute("posZoom").toInt()).
                setPosX(ele.attribute("posX").toInt()).
                setPosY(ele.attribute("posY").toInt()).
                setZoomMin(ele.attribute("zoomMin").toInt()).
                setZoomMax(ele.attribute("zoomMax").toInt()).
                setTileSize(ele.attribute("tileSize").toInt()).
                setFormat(ele.attribute("format"))
                .buildUnique();
        }
    }

    return map;
}


void Map2D::wheelEvent(QWheelEvent* event) {

    if(event->delta() > 0) {
        zoom += 0.2;
    } else {
        zoom -= 0.2;
    }

    // save initial numericZoom
    double  numZoomIni = numericZoom;

    // for tileProvider in tileProviders...
    // +1: better to downscale a tile than upscaling it, image will be sharper
    int zoomLevel = int(zoom)+1;
    zoomLevel = clamp(zoomLevel, tileProvider->getConfig()->zoomMin, tileProvider->getConfig()->zoomMax);
    numericZoom = zoom - zoomLevel;

    //double scaleFactor = numericZoom/numZoomIni;
    double scaleFactor = pow(2, numericZoom) / pow(2, numZoomIni);



    QPointF oldPos = mapToScene(event->pos());
    scale(scaleFactor, scaleFactor);

    Point2DLatLon latLon(Point2DTile(oldPos.x()/tileSize, oldPos.y()/tileSize, tileProvider->zoomLevel()));
    latLon.setZoom(zoomLevel);

    setPos(latLon, 0, 0);

    QPointF newPos = mapToScene(event->pos());
    Point2DTile ptt_new(newPos.x()/tileSize, newPos.y()/tileSize, tileProvider->zoomLevel());
    ptt_new.changeZoom(zoomLevel);

    Point2DTile ptt_old(latLon);
    QPointF newoldPos(ptt_old.x()*tileSize, ptt_old.y()*tileSize);
    std::cout << oldPos.x() << " " << oldPos.y()  << "    " << newoldPos.x() << " " << newoldPos.y() << std::endl;
    QPointF delta = newPos - newoldPos;
    translate(delta.x(), delta.y());

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

    int xMin = static_cast<int>(topLeft.x()/tileSize) - 2;
    int yMin = static_cast<int>(topLeft.y()/tileSize) - 2;
    int xMax = static_cast<int>(bottomRight.x()/tileSize) + 2;
    int yMax = static_cast<int>(bottomRight.y()/tileSize) + 2;

    for(int x=xMin; x<xMax; x++) {
        for(int y=yMin; y<yMax; y++) {
            Point2DTile coor(x, y, tileProvider->zoomLevel());
            tileProvider->fetch_tile(coor, coor);
        }
    }
}

void Map2D::acChanged(int ac_id) {
    (void)ac_id;
    setPos(Point2DLatLon(45.5, 1.34, 16));
}

void Map2D::handleTile(TileItem* tileReady, TileItem* tileObj) {
    if(tileReady->hasData()){
        if(!tileObj->isInScene()) {    // Not in scene, so lets add it
            scene->addItem(tileObj);
            tileObj->setZValue(tileProvider->zValue());
            tileObj->setInScene(true);
        }
        if(!tileObj->isVisible()) {    // in scene but hidden, lets show it. TODO: what if this slot is called just atfer a zoom change ?
            if(tileObj->coordinates().zoom() == tileProvider->zoomLevel()) {
                tileObj->show();
            }
        }

        QPointF pos = QPointF(
            tileSize*(tileObj->coordinates().x()),
            tileSize*(tileObj->coordinates().y())
        );
        tileObj->setPos(pos);
    } else {
        std::cout << "WHAAAAT ? Why I am receiving this signal? Non mais allo quoi !" << std::endl;
    }
}

void Map2D::setPos(Point2DLatLon latLon, double cx, double cy) {
    (void)cx;
    (void)cy;
    Point2DTile coorD(latLon);
    tileProvider->setZoomLevel(coorD.zoom());

    int xMin = coorD.xi() - width()/tileSize - 2;
    int xMax = coorD.xi() + width()/tileSize + 2;
    int yMin = coorD.yi() - height()/tileSize - 2;
    int yMax = coorD.yi() + height()/tileSize + 2;

    for(int x=xMin; x<=xMax; x++) {
        for(int y=yMin; y<=yMax; y++) {
            Point2DTile coor(x, y, tileProvider->zoomLevel());
            tileProvider->fetch_tile(coor, coor);
        }
    }

//    if(cx==0 && cy==0) {
//        cx = tileSize*coorD.x();
//        cy = tileSize*coorD.y();
//    }

//    centerOn(
//        QPointF(
//         cx,
//         cy
//     ));
}
