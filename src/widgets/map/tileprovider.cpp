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
    // a map for each zoom level so its easier to change zoom level
    for(int z=ZOOM_MIN; z<=ZOOM_MAX; z++) {
        tiles_maps.append(QMap<QString, TileItem*>());
    }

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
    }
    return QUrl(url.c_str());
}

void TileProvider::fetch_tile(Point2DTile t) {
    // If the file is beeing downloaded, do nothing, it will come soon !
    if(!downloading.contains(t.to_string())) {
        downloading.append(t.to_string());
        QMap<QString, TileItem*>::const_iterator tile = tiles_maps[_zoomLevel].find(t.to_string());
        if ( tile == tiles_maps[_zoomLevel].end() ) {
            // tile not in map. Load it from disk or download it
            std::string path = tilePath(t);
            std::ifstream f(path);
            if(f.good()) {
                // tile found on disk
                load_tile_from_disk(t);
            } else {
                // tile not on disk, download it

                QUrl url = tileUrl(t);

                QNetworkRequest request = QNetworkRequest(url);

                QList<QVariant> l = QList<QVariant>();
                l.append(t.x());
                l.append(t.y());
                l.append(t.zoom());
                request.setRawHeader("User-Agent", "Une belle tuile");
                request.setAttribute(QNetworkRequest::User, QVariant(l));
                manager->get(request);
            }

        } else {
            //tile found in map
            emit(tileReady(tile.value(), t));
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
        std::cout << "An Error occurs!!! " << reply->error() << std::endl;
    }

    downloading.removeAll(coor.to_string());
}


void TileProvider::load_tile_from_disk(Point2DTile t) {
    std::string path = tilePath(t);
    QPixmap pixmap = QPixmap(path.c_str());
    TileItem* item = new TileItem(pixmap, t);
    tiles_maps[_zoomLevel][t.to_string()] = item;
    emit(tileReady(item, t));
}

void TileProvider::setZoomLevel(int z) {
    if(z == _zoomLevel) {
        return; // nothing change
    }

    for(auto t: tiles_maps[_zoomLevel]) {
        t->hide();
    }

    if(z > ZOOM_MAX) {
        _zoomLevel = ZOOM_MAX;
    } else if(z < ZOOM_MIN) {
        _zoomLevel = ZOOM_MIN;
    } else {
        _zoomLevel = z;
    }
}
