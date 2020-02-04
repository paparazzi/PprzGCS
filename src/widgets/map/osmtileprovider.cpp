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

OSMTileProvider::OSMTileProvider(QObject *parent) : QObject (parent)
{
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


std::tuple<double, double> OSMTileProvider::LatlonFromTile(int x, int y, int z)
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
    std::cout << "Fetch tile " << "X" << std::get<0>(t) << " Y" << std::get<1>(t);
    QMap<TileCoorI, TileItem*>::const_iterator tile = tiles_map.find(t);
    if ( tile == tiles_map.end() ) {
        // tile not in map. Load it from disk or download it
        std::string path = tilePath(t);
        std::cout << "  search in " << path << std::endl;

        std::ifstream f(path);
        if(f.good()) {
            // tile found on disk
            load_tile_from_disk(t);
        } else {
            // tile not on disk, download it

            int x = std::get<0>(t);
            int y = std::get<1>(t);
            int z = std::get<2>(t);
            std::string url_str = "http://tile.openstreetmap.org/" +
                    std::to_string(z) + "/" +
                    std::to_string(x) + "/" +
                    std::to_string(y) + ".png";

            std::cout << "file NOT found! Download from " << url_str << std::endl;

            QUrl url = QUrl(url_str.c_str());

            QNetworkRequest request = QNetworkRequest(url);

            QList<QVariant> l = QList<QVariant>();
            l.append(x);
            l.append(y);
            l.append(z);
            request.setRawHeader("User-Agent", "Une belle tuile");
            request.setAttribute(QNetworkRequest::User, QVariant(l));
            manager->get(request);
        }

    } else {
        //tile found in map
        emit(tileReady(tile.value(), t));
    }
}

void OSMTileProvider::handleReply(QNetworkReply *reply) {
    QList<QVariant> l = reply->request().attribute(QNetworkRequest::User).toList();
    if(reply->error() == QNetworkReply::NetworkError::NoError) {
        int x = l.takeFirst().toInt(nullptr);
        int y = l.takeFirst().toInt(nullptr);
        int z = l.takeFirst().toInt(nullptr);
        TileCoorI coor = std::make_tuple(x, y, z);
        std::string path = tilePath(coor);

        std::cout << "save image to " << path << std::endl;

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
}


void OSMTileProvider::load_tile_from_disk(TileCoorI t) {
    std::string path = tilePath(t);
    std::cout << "file found!" << std::endl;
    QPixmap pixmap = QPixmap(path.c_str());
    TileItem* item = new TileItem(pixmap);
    tiles_map[t] = item;
    emit(tileReady(item, t));
}
