#include "srtm_manager.h"
#include <math.h>
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <QApplication>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include "gcs_utils.h"
#include <zip.h>

// with 1 arc-second resolution, there is 3600 samples per degree
#define SAMPLES_PER_DEG 3600
constexpr uint32_t TILE_SIZE = SAMPLES_PER_DEG + 1;

constexpr zip_uint64_t ZIP_LEN = TILE_SIZE*TILE_SIZE*2;

SRTMManager* SRTMManager::singleton = nullptr;
const QString SRTMManager::srtm_url = "https://step.esa.int/auxdata/dem/SRTMGL1/";

SRTMManager::SRTMManager(): QObject ()
{
    manager = new QNetworkAccessManager();
    diskCache = new QNetworkDiskCache();
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    manager->setCache(diskCache);
    connect(manager, &QNetworkAccessManager::finished, this, &SRTMManager::handleReply);
}

SRTMManager::~SRTMManager()
{
    for(auto tile: tiles) {
        delete tile.second;
    }
    tiles.clear();
}

void SRTMManager::load_srtm(double _lat_min, double _lat_max, double _lon_min, double _lon_max) {
    int lat_min = static_cast<int>(floor(_lat_min));
    int lat_max = static_cast<int>(ceil(_lat_max));
    int lon_min = static_cast<int>(floor(_lon_min));
    int lon_max = static_cast<int>(ceil(_lon_max));

    assert(lat_min < lat_max);
    assert(lon_min < lon_max);

    for(int lat=lat_min; lat<lat_max; lat++) {
        for(int lon=lon_min; lon<lon_max; lon++) {
            auto name = get_tile_name(lat, lon);
            load_tile(name);
        }
    }

}

QString SRTMManager::get_tile_name(int lat, int lon) {
    QString ss;
    ss += (lat>=0 ? "N":"S");
    ss += QString("%1").arg(abs(lat), 2, 10, QChar('0'));
    ss += (lon>=0 ? "E":"W");
    ss += QString("%1").arg(abs(lon), 3, 10, QChar('0'));
    return ss;
}


optional<int> SRTMManager::get_elevation(double lat, double lon) {
    if(tiles.size() == 0) { //quick easy check
        return nullopt;
    }

    double bottom = floor(lat);
    double left = floor(lon);
    auto name = get_tile_name(static_cast<int>(bottom), static_cast<int>(left));

    if(tiles.find(name) == tiles.end()) {
        return nullopt;
    }

    uint32_t y = static_cast<uint32_t>((lat-bottom)*SAMPLES_PER_DEG+0.5);
    uint32_t x = static_cast<uint32_t>((lon-left)*SAMPLES_PER_DEG+0.5);
    size_t pos = (2*((TILE_SIZE-y)*TILE_SIZE+x)) ;

    char* first = tiles[name];
    uint8_t msb = static_cast<uint8_t>(first[pos]);
    uint8_t lsb = static_cast<uint8_t>(first[pos + 1]);

    //Big-endian
    int16_t ele = (msb<<8)|lsb;

    // -32768 is the falg value when there is no data
    if(ele == -32768) {
        return nullopt;
    }

    return static_cast<int>(ele);
}

void SRTMManager::load_tile(QString name) {
    if(tiles.find(name) != tiles.end()) {
        return;
    }

    auto path = qApp->property("USER_DATA_PATH").toString() + "/srtm/" + name + ".SRTMGL1.hgt.zip";

    zip_t* zf = zip_open(path.toStdString().c_str(), ZIP_RDONLY, nullptr);

    if(zf != nullptr) {
        auto filename = name + ".hgt";

        zip_file_t* zip_file = zip_fopen(zf, filename.toStdString().c_str(), 0);

        if(zip_file != nullptr) {
            char* binaryData = new char[ZIP_LEN];
            zip_int64_t bytes_read = zip_fread(zip_file, binaryData, ZIP_LEN);
            assert(bytes_read == ZIP_LEN);
            tiles[name] = binaryData;
            int err = zip_fclose(zip_file);
            assert(err == 0);
        } else {
            qDebug() << "could not open " << filename << " in " << path;
        }

        // read only, don't save anything
        zip_discard(zf);

    } else {
        //tile is already being downloaded
        if(pending_downloads.contains(name)) {
            return;
        }
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(
                    nullptr,
                    "SRTM",
                    "Do you want to download SRTM tile " + name + "?",
                    QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QUrl url = srtm_url + name + ".SRTMGL1.hgt.zip";
            QNetworkRequest request = QNetworkRequest(url);
            request.setAttribute(QNetworkRequest::User, name);
            manager->get(request);
            pending_downloads.append(name);
        }
    }
}

void SRTMManager::handleReply(QNetworkReply *reply) {
    auto name = reply->request().attribute(QNetworkRequest::User).toString();
    if(reply->error() == QNetworkReply::NetworkError::NoError) {
        auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if(status.toInt() == 200) {
            auto path = qApp->property("USER_DATA_PATH").toString() + "/srtm/" + name + ".SRTMGL1.hgt.zip";
            QFile file(path);
            QFileInfo fi(path);
            QDir dirName = fi.dir();
            if(!dirName.exists()) {
                dirName.mkpath(dirName.path());
            }
            if(file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                reply->deleteLater();
                pending_downloads.removeAll(name);
                load_tile(name);
            }
        } else {
            qDebug() << "Error fetching SRTM tile, http status: " << status.toString();
        }
    } else {
        qDebug() << "Error fetching SRTM tile " << reply->errorString() << "! " << QString(reply->readAll());
    }
}
