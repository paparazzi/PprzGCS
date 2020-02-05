#include "osmtileprovider.h"
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

OSMTileProvider::OSMTileProvider(QObject *parent) : QObject (parent), _zoomLevel(16)
{
    // a map for each zoom level so its easier to change zoom level
    for(int z=ZOOM_MIN; z<=ZOOM_MAX; z++) {
        tiles_maps.append(QMap<TileCoorI, TileItem*>());
    }

    manager = new QNetworkAccessManager(this);
    diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    manager->setCache(diskCache);

    //connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleReply(QNetworkReply*)));
    connect(manager, &QNetworkAccessManager::finished, this, &OSMTileProvider::handleReply);
}

TileCoorD OSMTileProvider::tileCoorFromLatlon(double lat, double lon, int z)
{
    double tileX = (lon + 180.0) / 360.0 * (1 << z);
    double latrad = lat * M_PI/180.0;
    double tileY = (1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z);
    return std::make_tuple(tileX, tileY, z);
}


std::tuple<double, double> OSMTileProvider::LatlonFromTile(double x, double y, int z)
{
    double lon = x / static_cast<double>(1 << z) * 360.0 - 180;
    double n = M_PI - 2.0 * M_PI * y / static_cast<double>(1 << z);
    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    return std::make_tuple(lat, lon);
}

std::string OSMTileProvider::tilePath(TileCoorI coor) {
    std::string path = std::string(tilesPath) + "/" +
            std::to_string(std::get<2>(coor)) +
            "/X" + std::to_string(std::get<0>(coor)) +
            "_Y" + std::to_string(std::get<1>(coor)) + ".png";
    return path;
}

void OSMTileProvider::fetch_tile(TileCoorI t) {
    // If the file is beeing downloaded, do nothing, it will come soon !
    if(!downloading.contains(t)) {
        QMap<TileCoorI, TileItem*>::const_iterator tile = tiles_maps[_zoomLevel].find(t);
        if ( tile == tiles_maps[_zoomLevel].end() ) {
            // tile not in map. Load it from disk or download it
            std::string path = tilePath(t);
            std::ifstream f(path);
            if(f.good()) {
                // tile found on disk
                load_tile_from_disk(t);
            } else {
                // tile not on disk, download it

                int x = std::get<0>(t);
                int y = std::get<1>(t);
                int z = std::get<2>(t);
//                std::string url_str = "http://tile.openstreetmap.org/" +
//                        std::to_string(z) + "/" +
//                        std::to_string(x) + "/" +
//                        std::to_string(y) + ".png";
                std::string url_str = "https://khms3.google.com/kh/v=863?x=" +
                        std::to_string(x) + "&y=" +std::to_string(y) + "&z=" + std::to_string(z);

                QUrl url = QUrl(url_str.c_str());

                QNetworkRequest request = QNetworkRequest(url);

                QList<QVariant> l = QList<QVariant>();
                l.append(x);
                l.append(y);
                l.append(z);
                request.setRawHeader("User-Agent", "Une belle tuile");
                request.setAttribute(QNetworkRequest::User, QVariant(l));
                manager->get(request);
                downloading.append(t);
            }

        } else {
            //tile found in map
            emit(tileReady(tile.value(), t));
        }
    }
}

void OSMTileProvider::handleReply(QNetworkReply *reply) {
    QList<QVariant> l = reply->request().attribute(QNetworkRequest::User).toList();
    int x = l.takeFirst().toInt(nullptr);
    int y = l.takeFirst().toInt(nullptr);
    int z = l.takeFirst().toInt(nullptr);
    TileCoorI coor = std::make_tuple(x, y, z);

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
        std::cout << "An Error occurs!!! " << std::endl;
    }

    downloading.removeAll(coor);
}


void OSMTileProvider::load_tile_from_disk(TileCoorI t) {
    std::string path = tilePath(t);
    QPixmap pixmap = QPixmap(path.c_str());
    TileItem* item = new TileItem(pixmap, t);
    tiles_maps[_zoomLevel][t] = item;
    emit(tileReady(item, t));
}

void OSMTileProvider::setZoomLevel(int z) {
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
