#include "map2d.h"
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include <iostream>
#include "point2dlatlon.h"
#include "point2dtile.h"
#include <QtXml>
#include <QFile>
#include "utils.h"

Map2D::Map2D(QString configFile, QWidget *parent) : QGraphicsView(parent), numericZoom(0.0), _zoom(5.0), tileSize(256), minZoom(0.0), maxZoom(25.0)
{
    loadConfig(configFile);
    qreal maxxy = pow(2, maxZoom);

    _scene = new MapScene(-500, -500, tileSize*maxxy, tileSize*maxxy, parent);
    setScene(_scene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setBackgroundBrush(QBrush(Qt::darkGreen));

    Point2DLatLon initLatLon(43.462344,1.273044);
    centerLatLon(initLatLon);
}

void Map2D::toggleTileProvider(QString providerName, bool enable, int zValue, qreal opacity) {
    if(enable) {
        if(tile_providers.find(providerName) != tile_providers.end()) {
            tile_providers[providerName]->setVisible(true);
        } else {
            // create it
            if(tileSize == 0) {
                tileSize = sourceConfigs[providerName]->tileSize;
            }
            tile_providers[providerName] = new TileProvider(*sourceConfigs[providerName], zValue, tileSize, tiles_path, this);
            tile_providers[providerName]->setOpacity(opacity);
            connect(tile_providers[providerName], SIGNAL(displayTile(TileItem*, TileItem*)), this, SLOT(handleTile(TileItem*, TileItem*)));
        }
    }
    else {
        if(tile_providers.find(providerName) != tile_providers.end()) {
            tile_providers[providerName]->setVisible(false);
        } else {
            throw std::runtime_error("Can't desactivate something that don't exists!");
        }
    }
}

void Map2D::setLayerOpacity(QString providerName, qreal opacity) {
    if(tile_providers.find(providerName) != tile_providers.end()) {
        tile_providers[providerName]->setOpacity(opacity);
    }
}

void Map2D::setLayerZ(QString providerName, int z) {
    if(tile_providers.find(providerName) != tile_providers.end()) {
        tile_providers[providerName]->setZValue(z);
    }
}

void Map2D::setTilesPath(QString path) {
    tiles_path = path;
    for(auto tp: tile_providers) {
        tp.second->setTilesPath(tiles_path);
    }
}

bool Map2D::setTilesPath(QString path, QString providerName) {
    auto tp = tile_providers.find(providerName);
    if(tp != tile_providers.end()) {
        tp->second->setTilesPath(path);
        return true;
    }
    return false;
}

void Map2D::centerLatLon(Point2DLatLon latLon) {
    centerOn(scenePoint(latLon, zoomLevel()));
}

void Map2D::loadConfig(QString filename) {
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

            int zoomMin = ele.attribute("zoomMin", "0").toInt();
            int zoomMax = ele.attribute("zoomMax", "19").toInt();

            int xMin = ele.attribute("xMin", "0").toInt();
            int xMax = ele.attribute("xMax", "0").toInt();
            int yMin = ele.attribute("yMin", "0").toInt();
            int yMax = ele.attribute("xMax", "0").toInt();

            QString name = ele.attribute("name");
            sourceConfigs[name] = TileProviderConfig::builder{}.
                setName(name).
                setDir(ele.attribute("dir")).
                setAddr(ele.attribute("addr")).
                setPosZoom(ele.attribute("posZoom").toInt()).
                setPosX(ele.attribute("posX").toInt()).
                setPosY(ele.attribute("posY").toInt()).
                setZoomMin(zoomMin).
                setZoomMax(zoomMax).
                setXMin(xMin).
                setXMax(xMax).
                setYMin(yMin).
                setYMax(yMax).
                setTileSize(ele.attribute("tileSize").toInt()).
                setFormat(ele.attribute("format"))
                .newBuild();
        }
    }
}

void Map2D::resizeEvent(QResizeEvent *event){
    QGraphicsView::resizeEvent(event);
    updateTiles();
}

void Map2D::wheelEvent(QWheelEvent* event) {
    setResizeAnchor(QGraphicsView::NoAnchor);
    int curZoom = zoomLevel();

    if(event->delta() > 0) {
        _zoom += 0.5;
    } else {
        _zoom -= 0.5;
    }
    _zoom = clamp(_zoom, minZoom, maxZoom);

    // save initial numericZoom
    double  numZoomIni = numericZoom;

    // for tileProvider in tileProviders...
    int nextZoomLevel = zoomLevel();
    numericZoom = _zoom - nextZoomLevel;

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

    for(auto elt: tile_providers) {
        elt.second->setZoomLevel(nextZoomLevel);
    }

    updateTiles();
}

void Map2D::setZoom(double z) {
    QPointF center = mapToScene(QPoint(width()/2, height()/2));
    Point2DLatLon latLon(tilePoint(center, zoomLevel()));
    _zoom = z;
    updateTiles();
    centerLatLon(latLon);
}

void Map2D::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    if(event->buttons() & Qt::LeftButton) {
        updateTiles();
    }
}

void Map2D::updateTiles() {
    QPointF center = mapToScene(QPoint(width()/2,height()/2));

    int xCenter = static_cast<int>(center.x()/tileSize);
    int yCenter = static_cast<int>(center.y()/tileSize);
    int N = std::max(width(), height()) / (tileSize);

    for(auto elt: tile_providers) {
        TileProvider* tileProvider = elt.second;
        if(tileProvider->isVisible()) {

            Point2DTile coor = Point2DTile(xCenter, yCenter, zoomLevel());
            tileProvider->fetch_tile(coor, coor);

            for(int n=0; n<N+1; n++) {
                for(int i=-n; i<=n; i++) {
                    coor = Point2DTile(xCenter + i, yCenter + n, zoomLevel());
                    tileProvider->fetch_tile(coor, coor);
                    coor = Point2DTile(xCenter + i, yCenter - n, zoomLevel());
                    tileProvider->fetch_tile(coor, coor);
                }
                for(int j=1-n; j<n; j++) {
                    coor = Point2DTile(xCenter + n, yCenter + j, zoomLevel());
                    tileProvider->fetch_tile(coor, coor);
                    coor = Point2DTile(xCenter - n, yCenter + j, zoomLevel());
                    tileProvider->fetch_tile(coor, coor);
                }
            }
        }
    }
}

void Map2D::handleTile(TileItem* tileReady, TileItem* tileObj) {
    if(tileReady->hasData()){
        if(!tileObj->isInScene()) {    // Not in scene, so lets add it
            _scene->addItem(tileObj);
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

QList<QString> Map2D::tileProvidersNames() {
    QList<QString> names;
    for(auto tp:sourceConfigs) {
        names.append(tp.first);
    }
    return names;
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

Point2DLatLon Map2D::latlonPoint(QPointF scenePos, int zoom) {
    return Point2DLatLon(tilePoint(scenePos, zoom));
}

Point2DLatLon Map2D::latlonFromView(QPoint viewPos, int zoom) {
    return Point2DLatLon(tilePoint(mapToScene(viewPos), zoom));
}
