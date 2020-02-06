#include "tileprovider.h"
#include "math.h"
#include <iostream>
#include <string>
#include <fstream>
#include <QMap>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QNetworkProxy>

static const char tilesPath[] = "/home/fabien/DEV/test_qt/PprzGCS/data/map";

TileProvider::TileProvider(QObject *parent) : QObject (parent), _zoomLevel(16), source(GOOGLE)
{
    motherTile = new TileItem(nullptr);
    manager = new QNetworkAccessManager(this);
    diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    manager->setCache(diskCache);

    //connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleReply(QNetworkReply*)));
    connect(manager, &QNetworkAccessManager::finished, this, &TileProvider::handleReply);
}

std::string TileProvider::tilePath(Point2DTile coor) {
    std::string path;
    switch (source) {
    case GOOGLE:
        path += std::string(tilesPath) + "/GOOGLE/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".jpeg";
        break;
    case OSM_CLASSIC:
        path += std::string(tilesPath) + "/OSM_CLASSIC/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".png";
        break;
    case OSM_STAMEN:
        path += std::string(tilesPath) + "/OSM_STAMEN/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".jpg";
        break;
    case TERRAIN:
        path += std::string(tilesPath) + "/TERRAIN/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".png";
        break;
    case HIKING:
        path += std::string(tilesPath) + "/HIKING/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".png";
        break;
    }
    return path;
}

QUrl TileProvider::tileUrl(Point2DTile coor) {
    std::string url;

    switch (source) {
    case GOOGLE:
        url += "https://khms3.google.com/kh/v=863?x=" +
                std::to_string(coor.xi()) + "&y=" +std::to_string(coor.yi()) + "&z=" + std::to_string(coor.zoom());
        break;
    case OSM_CLASSIC:
        url += "http://tile.openstreetmap.org/" +
                std::to_string(coor.zoom()) + "/" +
                std::to_string(coor.xi()) + "/" +
                std::to_string(coor.yi()) + ".png";
        break;
    case OSM_STAMEN:
        url += "http://c.tile.stamen.com/watercolor/" +
                std::to_string(coor.zoom()) + "/" +
                std::to_string(coor.xi()) + "/" +
                std::to_string(coor.yi()) + ".jpg";
        break;
    case TERRAIN:
        url += "http://b.tile.stamen.com/terrain/" +
                std::to_string(coor.zoom()) + "/" +
                std::to_string(coor.xi()) + "/" +
                std::to_string(coor.yi()) + ".png";
        break;
    case HIKING:
        url += "https://tile.waymarkedtrails.org/hiking/" +
                std::to_string(coor.zoom()) + "/" +
                std::to_string(coor.xi()) + "/" +
                std::to_string(coor.yi()) + ".png";
        break;
    }
    return QUrl(url.c_str());
}

void TileProvider::fetch_tile(Point2DTile t) {
    // If the file is beeing downloaded, do nothing, it will come soon !
    if(t.isValid() && !downloading.contains(t.to_istring())) {
        TileItem* tile = getTile(t);
        if (!tile->hasData()) {
            // tile not in map. Load it from disk or download it
            std::string path = tilePath(t);
            std::ifstream f(path);
            if(f.good()) {
                // tile found on disk
                load_tile_from_disk(t);
            } else {
                // tile not on disk, download it
                downloading.append(t.to_istring());
                QUrl url = tileUrl(t);

                QNetworkRequest request = QNetworkRequest(url);

                QList<QVariant> l = QList<QVariant>();
                l.append(t.x());
                l.append(t.y());
                l.append(t.zoom());
                request.setRawHeader("User-Agent", "PPRZGCS");
                request.setAttribute(QNetworkRequest::User, QVariant(l));
                manager->get(request);
            }

        } else {
            //tile found in map
            emit(tileReady(tile, t));
        }
    }
}

void TileProvider::handleReply(QNetworkReply *reply) {
    QList<QVariant> l = reply->request().attribute(QNetworkRequest::User).toList();
    int x = l.takeFirst().toInt(nullptr);
    int y = l.takeFirst().toInt(nullptr);
    int z = l.takeFirst().toInt(nullptr);
    Point2DTile coor(x, y, z);

    if(reply->error() == QNetworkReply::NetworkError::NoError) {
        std::string path = tilePath(coor);
        QFile file(path.c_str());
        QFileInfo fi(path.c_str());
        QDir dirName = fi.dir();
        if(!dirName.exists()) {
            dirName.mkpath(dirName.path());
        }

        if(file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            reply->deleteLater();
        }
        load_tile_from_disk(coor);

    } else {
        TileItem* tile = getTile(coor);
        int xi = tile->coordinates().xi() & 1;
        int yi = tile->coordinates().yi() & 1;

        QRect rect(xi*TILE_SIZE/2, yi*TILE_SIZE/2, TILE_SIZE/2, TILE_SIZE/2);
        QPixmap cropped = tile->mother()->pixmap().copy(rect);
        //cropped.scaledToWidth(TILE_SIZE);
        tile->setAltPixmap(cropped);
        tile->setScale(2);

        if(reply->error() == QNetworkReply::NetworkError::ConnectionRefusedError) {
            std::cout << "ConnectionRefusedError! Maybe the tile provider banned you ?" << std::endl;
        } else {
           std::cout << "Error " << reply->error() << " !" << coor.to_istring().toStdString() << std::endl;
        }
    }

    downloading.removeAll(coor.to_istring());
}


void TileProvider::load_tile_from_disk(Point2DTile t) {
    std::string path = tilePath(t);
    TileItem* item = getTile(t);
    item->setPixmap(QPixmap(path.c_str()));
    emit(tileReady(item, t));
}

void TileProvider::setZoomLevel(int z) {
    if(z == _zoomLevel) {
        return; // nothing change
    }

//    for(auto t: tiles_maps[_zoomLevel]) {
//        t->hide();
//    }

    TileIterator iter(motherTile);
    while(true) {
        TileItem* tile = iter.next();
        if(tile == nullptr) {
            break;
        }
        if(tile->hasData() && tile->coordinates().zoom()==_zoomLevel) {
            tile->hide();
        }
    }

    if(z > ZOOM_MAX) {
        _zoomLevel = ZOOM_MAX;
    } else if(z < ZOOM_MIN) {
        _zoomLevel = ZOOM_MIN;
    } else {
        _zoomLevel = z;
    }
}

TileItem* TileProvider::getTile(Point2DTile p) {
    TileItem* current = motherTile;

    // mask to apply to the full path (to the objective tile 'p') to get the partial path (the 'next' tile)
    unsigned int mask = 0;

    for(int i=p.zoom()-1; i>=0; i--) {
        int xi = (p.xi() & 1<<i) ? 1 : 0;
        int yi = (p.yi() & 1<<i) ? 1 : 0;
        mask |= 1<<i;

        TileItem* next = current->child(xi, yi);

        if(next == nullptr) {
            next = new TileItem(current, Point2DTile(p.xi()&mask, p.yi()&mask, p.zoom()-i));
            current->setChild(next, xi, yi);
        }

        current = next;
    }

    return current;
}
