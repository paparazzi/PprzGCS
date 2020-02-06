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
#include <QGraphicsScene>

static const char tilesPath[] = "/home/fabien/DEV/test_qt/PprzGCS/data/map";

TileProvider::TileProvider(QObject *parent) : QObject (parent), _zoomLevel(16), source(GOOGLE)
{
    motherTile = new TileItem(nullptr, TILE_SIZE);
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
    case IGN:
        path += std::string(tilesPath) + "/IGN/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".jpeg";
        break;
    case ICAO:
        path += std::string(tilesPath) + "/ICAO/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".jpeg";
        break;
    case FRANCE_DRONE_RESTRICTIONS:
        path += std::string(tilesPath) + "/FR_UAV_RESTRICT/" +
                std::to_string(coor.zoom()) +
                "/X" + std::to_string(coor.xi()) +
                "_Y" + std::to_string(coor.yi()) + ".jpeg";
        break;
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
    case IGN:
        url += "https://wxs.ign.fr/an7nvfzojv5wa96dsga5nk8w/geoportail/wmts?layer=ORTHOIMAGERY.ORTHOPHOTOS&style=normal&tilematrixset=PM&Service=WMTS&Request=GetTile&Version=1.0.0&Format=image%2Fjpeg&TileMatrix=" +
                std::to_string(coor.zoom()) + "&TileCol=" +
                std::to_string(coor.xi()) + "&TileRow=" +
                std::to_string(coor.yi());
                break;
    case ICAO:
        url += "https://wxs.ign.fr/an7nvfzojv5wa96dsga5nk8w/geoportail/wmts?layer=GEOGRAPHICALGRIDSYSTEMS.MAPS.SCAN-OACI&style=normal&tilematrixset=PM&Service=WMTS&Request=GetTile&Version=1.0.0&Format=image%2Fjpeg&TileMatrix=" +
                std::to_string(coor.zoom()) + "&TileCol=" +
                std::to_string(coor.xi()) + "&TileRow=" +
                std::to_string(coor.yi());
                break;
    case FRANCE_DRONE_RESTRICTIONS:
        url += "https://wxs.ign.fr/an7nvfzojv5wa96dsga5nk8w/geoportail/wmts?layer=TRANSPORTS.DRONES.RESTRICTIONS&style=normal&tilematrixset=PM&Service=WMTS&Request=GetTile&Version=1.0.0&Format=image%2Fpng&TileMatrix=" +
                std::to_string(coor.zoom()) + "&TileCol=" +
                std::to_string(coor.xi()) + "&TileRow=" +
                std::to_string(coor.yi());
                break;
        break;
    }
    return QUrl(url.c_str());
}

void TileProvider::fetch_tile(Point2DTile t, Point2DTile tObj) {
    if(t.isValid() && tObj.isValid()) {
        TileItem* tile = getTile(t);
        TileItem* tileObj = getTile(tObj);
        if(!tile->dataGood()) {
            // display it, or a parent tile if one exist
            TileItem* current = tile;
            while(current != nullptr) {
                // tile found on disk
                bool success = load_tile_from_disk(current);
                if(success) {
                    if(/*!tileObj->hasData() && */current != tileObj) {    // an ancestor was loaded. inherit its data for tileObj
                        tileObj->setInheritedData();
                    }
                    emit(displayTile(current, tileObj));
                    break;
                } else {
                    // this tile was not on the disk, so try with its mother
                    current = current->mother();
                }

            }
            if(current == nullptr) {
                std::cout << "No ancestor found!!!" << std::endl;
            }
            // if the tile was not found, dl it
            if(current != tile) {
                //dl tile
//                // tile not on disk, download it
                //downloading.append(t.to_istring());
                QUrl url = tileUrl(t);
                QNetworkRequest request = QNetworkRequest(url);


                // tuple : what is dl, what we want to display
                downloading.append(std::make_tuple(tile, tile));
                request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:72.0) Gecko");
                QList<QVariant> l = QList<QVariant>();
                l.push_back(QVariant::fromValue(tile));
                l.push_back(QVariant::fromValue(tile));

                request.setAttribute(QNetworkRequest::User, l);
                manager->get(request);


            }

        } else {
            //tile with data found in tree
            emit(displayTile(tile, tile));
        }

    }
}

void TileProvider::handleReply(QNetworkReply *reply) {
    QList<QVariant> l = reply->request().attribute(QNetworkRequest::User).toList();

    TileItem* tileCur = l.takeFirst().value<TileItem*>();
    TileItem* tileObj = l.takeFirst().value<TileItem*>();

    if(reply->error() == QNetworkReply::NetworkError::NoError) {
        std::string path = tilePath(tileCur->coordinates());
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

        bool success = load_tile_from_disk(tileCur);
        if(success) {
            if(/*!tileObj->hasData() && */tileCur != tileObj) {    // an ancestor was loaded. inherit its data for tileObj
                tileObj->setInheritedData();
            }
            emit(displayTile(tileCur, tileObj));
        } else {
           //whyyyyy ???
            std::cout << "WHYYYYYYYYYYY ?" << std::endl;
        }


    } else {
        //tile dl failed. try the parent tile ?
        TileItem* parentTile = tileCur->mother();
        if(parentTile != nullptr) {
            fetch_tile(tileCur->mother()->coordinates(), tileObj->coordinates());
        }

        std::cout << "Error " << reply->error() << " ! " << reply->readAll().toStdString() << std::endl;
    }

}


bool TileProvider::load_tile_from_disk(TileItem* item) {
    std::string path = tilePath(item->coordinates());
    std::ifstream f(path);
    if(f.good()) {
        // tile found on disk
        item->setPixmap(QPixmap(path.c_str()));
        return true;
    } else {
        return false;
    }
}

void TileProvider::setZoomLevel(int z) {
    if(z == _zoomLevel) {
        return; // nothing change
    }

    if(z > ZOOM_MAX) {
        _zoomLevel = ZOOM_MAX;
    } else if(z < ZOOM_MIN) {
        _zoomLevel = ZOOM_MIN;
    } else {
        _zoomLevel = z;
    }

    //TODO improve iterator usability (make a C++ standard one)
    TileIterator iter(motherTile);
    while(true) {
        TileItem* tile = iter.next();
        if(tile == nullptr) {
            break;
        }
        if(tile->hasData() && tile->coordinates().zoom() != _zoomLevel) {
            tile->hide();
        }
    }

}

TileItem* TileProvider::getTile(Point2DTile p) {
    TileItem* current = motherTile;

    // mask to apply to the full path (to the objective tile 'p') to get the partial path (the 'next' tile)
    int mask = 0;

    for(int i=p.zoom()-1; i>=0; i--) {
        int xi = (p.xi() & 1<<i) ? 1 : 0;
        int yi = (p.yi() & 1<<i) ? 1 : 0;
        mask |= 1<<i;

        TileItem* next = current->child(xi, yi);

        if(next == nullptr) {
            int x = (p.xi() & mask) >> i;
            int y = (p.yi() & mask) >> i;
            int zoom = p.zoom()-i;

            next = new TileItem(current, motherTile->tileSize(), Point2DTile(x, y, zoom));
            current->setChild(next, xi, yi);
        }

        current = next;
    }

    return current;
}
