#include "map2d.h"
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <iostream>
#include "point2dlatlon.h"
#include "point2dtile.h"
#include <QtXml>
#include <QFile>
#include "utils.h"
#include "maputils.h"
#include "coordinatestransform.h"
#include <QApplication>
#include "gcs_utils.h"

Map2D::Map2D(QWidget *parent) : QGraphicsView(parent),
    numericZoom(0.0), _zoom(5.0), tile_size(256), minZoom(0.0), maxZoom(25.0), wheelAccumulator(0)
{
    QString configFile = user_or_app_path("tile_sources.xml");
    loadConfig(configFile);
    qreal maxxy = pow(2, maxZoom);

    _scene = new MapScene(-500, -500, tile_size*maxxy, tile_size*maxxy, parent);
    setScene(_scene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setBackgroundBrush(QBrush(QColor(0x151515)));

    Point2DLatLon initLatLon(43.462344,1.273044);
}

void Map2D::toggleTileProvider(QString providerName, bool enable, int zValue, qreal opacity) {
    if(enable) {
        if(tile_providers.find(providerName) != tile_providers.end()) {
            tile_providers[providerName]->setVisible(true);
        } else {
            // create it
            if(tile_providers.size() == 0) {
                tile_size = sourceConfigs[providerName]->tileSize;
            }
            tile_providers[providerName] = new TileProvider(*sourceConfigs[providerName], zValue, tile_size, tiles_path, this);
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
    for(auto &tp: tile_providers) {
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
    centerOn(scenePoint(latLon, zoomLevel(_zoom), tile_size));
    updateTiles();
}

/**
 * @brief Map2D::zoomBox
 * @param nw
 * @param se
 * @return max zoom fitting nw and se.
 */
double Map2D::zoomBox(Point2DLatLon nw, Point2DLatLon se) {
    double targetZoom = 0;
    while(targetZoom < 25) {
        Point2DTile pnw(nw, zoomLevel(targetZoom));
        Point2DTile pse(se, zoomLevel(targetZoom));
        double sf = pow(2, targetZoom - zoomLevel(targetZoom));
        int dx = (pse.x() - pnw.x()) * tileSize() / sf;
        int dy = (pse.y() - pnw.y()) * tileSize() / sf;

        if(dx >= viewport()->rect().width() || dy >= viewport()->rect().height()) {
            break;
        }
        targetZoom += 0.5;
    }
    return targetZoom;
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
            auto tpc = new TileProviderConfig(ele);
            sourceConfigs[tpc->name] = tpc;
        }
    }
}

void Map2D::resizeEvent(QResizeEvent *event){
    QGraphicsView::resizeEvent(event);
    updateTiles();
}

void Map2D::wheelEvent(QWheelEvent* event) {
    setResizeAnchor(QGraphicsView::NoAnchor);

    wheelAccumulator += event->angleDelta().y();
    event->accept();

    if(qAbs(wheelAccumulator) < 120) {
        return;
    }

    if(wheelAccumulator > 0) {
        zoomCentered(_zoom + 0.5, event->pos());
    } else {
        zoomCentered(_zoom - 0.5, event->pos());
    }
    wheelAccumulator = 0;
}

void Map2D::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Plus) {
        zoomCentered(_zoom + 0.5, QPoint(width()/2, height()/2));
        event->accept();
    } else if (event->key() == Qt::Key_Minus) {
        zoomCentered(_zoom - 0.5, QPoint(width()/2, height()/2));
        event->accept();
    } else {
        event->ignore();
    }
}

void Map2D::setZoom(double z) {
    zoomCentered(z, QPoint(width()/2, height()/2));
}

void Map2D::zoomCentered(double z, QPoint eventPos) {
    int curZoom = zoomLevel(_zoom);
    _zoom = clamp(z, minZoom, maxZoom);

    // save initial numericZoom
    double  numZoomIni = numericZoom;

    // for tileProvider in tileProviders...
    int nextZoomLevel = zoomLevel(_zoom);
    numericZoom = _zoom - nextZoomLevel;

    double scaleFactor = pow(2, numericZoom) / pow(2, numZoomIni);

    // mouse pos in scene
    QPointF oldPos = mapToScene(eventPos);
    // lat lon point pointed by the mouse (at the current zoomLevel)
    auto poi = CoordinatesTransform::get()->wgs84_from_scene(oldPos, curZoom, tile_size);

    scale(scaleFactor, scaleFactor);    // apply scale
    // mouse pos in scene after scale
    QPointF newPos = mapToScene(eventPos);
    // position of the poi in scene coordinates for the new zoom
    QPointF poi_scene = scenePoint(poi, nextZoomLevel, tile_size);

    QPointF delta = newPos - poi_scene;
    translate(delta.x(), delta.y());

    for(auto &elt: tile_providers) {
        elt.second->setZoomLevel(nextZoomLevel);
    }

    updateTiles();

}

void Map2D::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    if(event->buttons() & mouse_load_tiles_mask) {
        updateTiles();
    }
}

void Map2D::updateTiles() {
    QPointF center = mapToScene(QPoint(width()/2,height()/2));

    int xCenter = static_cast<int>(center.x()/tile_size);
    int yCenter = static_cast<int>(center.y()/tile_size);
    int N = ceil(std::max(width(), height()) / (2.0 * tile_size)) + 1;

    for(auto &elt: tile_providers) {
        TileProvider* tileProvider = elt.second;
        if(tileProvider->isVisible()) {

            Point2DTile coor = Point2DTile(xCenter, yCenter, zoomLevel(_zoom));
            tileProvider->fetch_tile(coor, coor);

            for(int n=0; n<=N; n++) {
                for(int i=-n; i<=n; i++) {
                    coor = Point2DTile(xCenter + i, yCenter + n, zoomLevel(_zoom));
                    tileProvider->fetch_tile(coor, coor);
                    coor = Point2DTile(xCenter + i, yCenter - n, zoomLevel(_zoom));
                    tileProvider->fetch_tile(coor, coor);
                }
                for(int j=1-n; j<n; j++) {
                    coor = Point2DTile(xCenter + n, yCenter + j, zoomLevel(_zoom));
                    tileProvider->fetch_tile(coor, coor);
                    coor = Point2DTile(xCenter - n, yCenter + j, zoomLevel(_zoom));
                    tileProvider->fetch_tile(coor, coor);
                }
            }
        }
    }
}

void Map2D::getViewPoints(Point2DLatLon& nw, Point2DLatLon& se) {
    QPointF top_left = mapToScene(QPoint(0,0));
    QPointF bottom_right = mapToScene(QPoint(width(),height()));
    nw = CoordinatesTransform::get()->wgs84_from_scene(top_left, zoomLevel(zoom()), tile_size);
    se = CoordinatesTransform::get()->wgs84_from_scene(bottom_right, zoomLevel(zoom()), tile_size);
}


void Map2D::handleTile(TileItem* tileReady, TileItem* tileObj) {
    if(tileReady->hasData()){
        if(!tileObj->isInScene()) {    // Not in scene, so lets add it
            _scene->addItem(tileObj);
            tileObj->setInScene(true);
        }
        if(!tileObj->isVisible()) {    // in scene but hidden, lets show it. TODO: what if this slot is called just atfer a zoom change ?
            if(tileObj->coordinates().zoom() == zoomLevel(_zoom)) {
                tileObj->show();
            }
        }

        QPointF pos = QPointF(
            tile_size*(tileObj->coordinates().x()),
            tile_size*(tileObj->coordinates().y())
        );
        tileObj->setPos(pos);
    } else {
        std::cout << "WHAAAAT ? Why I am receiving this signal but I have no data ?" << std::endl;
    }
}

QList<QString> Map2D::tileProvidersNames() {
    QList<QString> names;
    for(auto &tp: sourceConfigs) {
        names.append(tp.first);
    }
    return names;
}

Point2DLatLon Map2D::latlonFromView(QPoint viewPos, int zoom) {
    return CoordinatesTransform::get()->wgs84_from_scene(mapToScene(viewPos), zoom, tile_size);
}
