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
#include "utils.h"

Map2D::Map2D(QWidget *parent) : QGraphicsView(parent), numericZoom(0.0), zoom(16.0), minZoom(0.0), maxZoom(21.0)
{
    sourceConfigs = loadConfig("://tile_sources.xml");
    auto& config = sourceConfigs[QString("Google")];
    config->printConfig();
    tileSize = config->tileSize;

    int maxxy = 1 << static_cast<int>(maxZoom);

    scene = new QGraphicsScene(-500, -500, tileSize*maxxy, tileSize*maxxy, parent);
    setScene(scene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setBackgroundBrush(QBrush(Qt::darkGreen));

    tileProvider = new TileProvider(config, 0, tileSize, this);

    connect(DispatcherUi::get(), SIGNAL(ac_selected(int)), this, SLOT(acChanged(int)));

    connect(tileProvider, SIGNAL(displayTile(TileItem*, TileItem*)), this, SLOT(handleTile(TileItem*, TileItem*)));

    Point2DLatLon initLatLon(43.4625, 1.2732);

    centerLatLon(initLatLon);
}

void Map2D::centerLatLon(Point2DLatLon latLon) {
    centerOn(scenePoint(latLon, zoomLevel()));
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

void Map2D::resizeEvent(QResizeEvent *event){
    QGraphicsView::resizeEvent(event);
    updateTiles();
}

void Map2D::wheelEvent(QWheelEvent* event) {
    setResizeAnchor(QGraphicsView::NoAnchor);
    int curZoom = zoomLevel();

    if(event->delta() > 0) {
        zoom += 0.5;
    } else {
        zoom -= 0.5;
    }
    zoom = clamp(zoom, minZoom, maxZoom);

    // save initial numericZoom
    double  numZoomIni = numericZoom;

    // for tileProvider in tileProviders...
    int nextZoomLevel = zoomLevel();
    numericZoom = zoom - nextZoomLevel;

    double scaleFactor = pow(2, numericZoom) / pow(2, numZoomIni);

    // mouse pos in scene
    QPointF oldPos = mapToScene(event->pos());
    // lat lon point pointed by the mouse (at the current zoomLevel)
    Point2DLatLon poi(tilePoint(oldPos, curZoom));

    scale(scaleFactor, scaleFactor);    // apply scale
    // mouse pos in scene after scale
    QPointF newPos = mapToScene(event->pos());
    // position of the poi in scene coordinates for the new zoom
    QPointF poi_scene = scenePoint(poi, nextZoomLevel);

    QPointF delta = newPos - poi_scene;
    translate(delta.x(), delta.y());

    tileProvider->setZoomLevel(nextZoomLevel);
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

    int xMin = static_cast<int>(topLeft.x()/tileSize);
    int yMin = static_cast<int>(topLeft.y()/tileSize);
    int xMax = static_cast<int>(bottomRight.x()/tileSize)+1;
    int yMax = static_cast<int>(bottomRight.y()/tileSize)+1;

    for(int x=xMin; x<xMax; x++) {
        for(int y=yMin; y<yMax; y++) {
            Point2DTile coor = Point2DTile(x, y, zoomLevel());
            tileProvider->fetch_tile(coor, coor);
        }
    }
}

void Map2D::acChanged(int ac_id) {
    (void)ac_id;
}

void Map2D::handleTile(TileItem* tileReady, TileItem* tileObj) {
    if(tileReady->hasData()){
        if(!tileObj->isInScene()) {    // Not in scene, so lets add it
            scene->addItem(tileObj);
            tileObj->setZValue(tileProvider->zValue());
            tileObj->setInScene(true);
        }
        if(!tileObj->isVisible()) {    // in scene but hidden, lets show it. TODO: what if this slot is called just atfer a zoom change ?
            if(tileObj->coordinates().zoom() == zoomLevel()) {
                tileObj->show();
            }
        }

        QPointF pos = QPointF(
            tileSize*(tileObj->coordinates().x()),
            tileSize*(tileObj->coordinates().y())
        );
        tileObj->setPos(pos);
    } else {
        std::cout << "WHAAAAT ? Why I am receiving this signal but I have no data ?" << std::endl;
    }
}

Point2DTile Map2D::tilePoint(QPointF scenePos, int zoom) {
    return Point2DTile(scenePos.x()/tileSize, scenePos.y()/tileSize, zoom);
}

QPointF Map2D::scenePoint(Point2DTile tilePoint) {
    return QPointF(tilePoint.x()*tileSize, tilePoint.y()*tileSize);
}

QPointF Map2D::scenePoint(Point2DLatLon latlon, int zoomLvl) {
    Point2DTile tile_pos = Point2DTile(latlon, zoomLvl);
    return scenePoint(tile_pos);
}
