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
#include <QtWidgets>
#include <QSettings>

// with 1 arc-second resolution, there is 3600 samples per degree
#define SAMPLES_PER_DEG 3600
constexpr uint32_t TILE_SIZE = SAMPLES_PER_DEG + 1;

constexpr zip_uint64_t ZIP_LEN = TILE_SIZE*TILE_SIZE*2;

const QString SRTMManager::srtm_url = "https://step.esa.int/auxdata/dem/SRTMGL1/";

SRTMManager::SRTMManager(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{
    manager = new QNetworkAccessManager();
    diskCache = new QNetworkDiskCache();
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    manager->setCache(diskCache);
    connect(manager, &QNetworkAccessManager::finished, this, &SRTMManager::handleReply);
}

SRTMManager::~SRTMManager()
{
    for(auto &tile: tiles) {
        delete tile.second;
    }
    tiles.clear();
}

QList<QString> SRTMManager::get_tile_names(double _lat_min, double _lat_max, double _lon_min, double _lon_max) {
    int lat_min = static_cast<int>(floor(_lat_min));
    int lat_max = static_cast<int>(ceil(_lat_max));
    int lon_min = static_cast<int>(floor(_lon_min));
    int lon_max = static_cast<int>(ceil(_lon_max));

    if(lat_min > lat_max || lon_min > lon_max) {
        qCritical() << "invalid range";
        return QList<QString>();
    }

    QList<QString> names;

    for(int lat=lat_min; lat<lat_max; lat++) {
        for(int lon=lon_min; lon<lon_max; lon++) {
            names.append(get_tile_name(lat, lon));
        }
    }
    return names;
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

void SRTMManager::load_tiles(QList<QString> names, bool local_only) {

    // load all tiles that are on the disk
    QList<QString> tiles_dl;
    for(auto &name: names) {
        if(!load_tile(name, false)) {
            tiles_dl.append(name);
        }
    }

    // open the dialog for the tiles to be downloaded
    if(tiles_dl.size() > 0 && !local_only) {
        auto dialog = new SRTMDialog(names, tiles_dl, QApplication::activeWindow());

        connect(dialog, &SRTMDialog::tilesConfirmed, this, [=,this](QList<QString> tiles_confirmed) {
            for(auto &name: tiles_confirmed) {
                load_tile(name, true);
            }
        });

        dialog->showMovable();
    }
}

int SRTMManager::load_tile(QString name, bool dl) {
    if(tiles.find(name) != tiles.end()) {
        return true;
    }
    auto settings = getAppSettings();

    auto path = appConfig()->value("USER_DATA_PATH").toString() + "/srtm/" + name + ".SRTMGL1.hgt.zip";

    zip_t* zf = zip_open(path.toStdString().c_str(), ZIP_RDONLY, nullptr);

    if(zf != nullptr) {
        auto filename = name + ".hgt";

        zip_file_t* zip_file = zip_fopen(zf, filename.toStdString().c_str(), 0);

        if(zip_file != nullptr) {
            char* binaryData = new char[ZIP_LEN];
            zip_int64_t bytes_read = zip_fread(zip_file, binaryData, ZIP_LEN);
            zip_fclose(zip_file);
            if(bytes_read == ZIP_LEN) {
                tiles[name] = binaryData;
            } else {
                qDebug() << "could not read " << filename << " in " << path;
                delete[] binaryData;
            }
        } else {
            qDebug() << "could not open " << filename << " in " << path;
        }

        // read only, don't save anything
        zip_discard(zf);

        return true;

    } else {
        //tile is already being downloaded
        if(pending_downloads.contains(name)) {
            return true;
        }

        if(dl) {
            QUrl url = srtm_url + name + ".SRTMGL1.hgt.zip";
            QNetworkRequest request = QNetworkRequest(url);
            request.setAttribute(QNetworkRequest::User, name);
            auto reply = manager->get(request);
            connect(reply, &QNetworkReply::downloadProgress, this, [=,this](qint64 received, qint64 total) {
                emit downloadProgress(name, received, total);
            });
            pending_downloads.append(name);
            return true;
        } else {
            return false;
        }

    }
}

void SRTMManager::handleReply(QNetworkReply *reply) {
    auto name = reply->request().attribute(QNetworkRequest::User).toString();
    reply->deleteLater();
    pending_downloads.removeAll(name);  // also on errors: it can be tried again

    QString error;
    if(reply->error() == QNetworkReply::NetworkError::NoError) {
        auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if(status.toInt() == 200) {
            auto path = appConfig()->value("USER_DATA_PATH").toString() + "/srtm/" + name + ".SRTMGL1.hgt.zip";
            QFile file(path);
            QFileInfo fi(path);
            QDir dirName = fi.dir();
            if(!dirName.exists()) {
                dirName.mkpath(dirName.path());
            }
            if(file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                load_tile(name, false);
            } else {
                error = "can't write " + path;
            }
        } else {
            error = "http status " + status.toString();
        }
    } else {
        error = reply->errorString();
    }
    if(!error.isEmpty()) {
        qDebug() << "Error fetching SRTM tile" << name << ":" << error;
    }
    emit downloadFinished(name, error.isEmpty(), error);
}


SRTMDialog::SRTMDialog(QList<QString> tiles_names, QList<QString> missing, QWidget *parent) :
    CardDialog("Ground height (SRTM)", "flight plans and map view", parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    addText(QString("%1 %2 the flight plans and the map view, %3 missing:")
            .arg(tiles_names.size()).arg(tiles_names.size() == 1 ? "tile covers" : "tiles cover").arg(missing.size()));

    auto grid = new QGridLayout();
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(1, 1);
    body()->addLayout(grid);

    QMap<QString, QCheckBox*> checks;
    QMap<QString, QLabel*> states;
    QMap<QString, QProgressBar*> bars;
    // missing tiles first
    QList<QString> ordered = missing;
    for(auto &n: tiles_names) {
        if(!ordered.contains(n)) {
            ordered.append(n);
        }
    }
    int r = 0;
    for(auto &name: ordered) {
        auto state = new QLabel(this);
        state->setObjectName("card_muted");
        auto name_label = new QLabel(name, this);
        name_label->setStyleSheet("font-family: Monospace;");
        if(missing.contains(name)) {
            auto ck = new QCheckBox(this);
            ck->setChecked(true);
            checks[name] = ck;
            grid->addWidget(ck, r, 0);
            state->setText(SRTMManager::get()->isDownloading(name) ? "downloading" : "missing");
        } else {
            auto ok = new QLabel(QString::fromUtf8("✓"), this);
            ok->setStyleSheet("color: #1a7f37; font-weight: bold;");
            grid->addWidget(ok, r, 0, Qt::AlignCenter);
            name_label->setObjectName("card_muted");
            state->setText("on this computer");
        }
        grid->addWidget(name_label, r, 1);
        grid->addWidget(state, r, 2, Qt::AlignRight);
        states[name] = state;
        r++;
        if(missing.contains(name)) {
            auto bar = new QProgressBar(this);
            bar->setTextVisible(false);
            bar->setFixedHeight(6);
            bar->setRange(0, 0);
            bar->setVisible(false);
            grid->addWidget(bar, r, 1, 1, 2);
            bars[name] = bar;
            r++;
        }
    }

    auto close = addButton("Close");
    auto download = addButton("", PRIMARY, 2);

    auto checked = [=]() {
        QList<QString> l;
        for(auto it = checks.begin(); it != checks.end(); ++it) {
            if(it.value()->isEnabled() && it.value()->isChecked()) {
                l.append(it.key());
            }
        }
        return l;
    };
    auto update_button = [=]() {
        auto n = checked().size();
        download->setText(n == 1 ? QString("Download 1 tile") : QString("Download %1 tiles").arg(n));
        download->setEnabled(n > 0);
    };
    for(auto ck: checks) {
        connect(ck, &QCheckBox::toggled, this, update_button);
    }
    update_button();

    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    connect(download, &QPushButton::clicked, this, [=,this]() {
        auto names = checked();
        for(auto &name: names) {
            checks[name]->setEnabled(false);
            states[name]->setText("starting");
            states[name]->setStyleSheet("");
            bars[name]->setRange(0, 0);
            bars[name]->setVisible(true);
        }
        emit tilesConfirmed(names);
        update_button();
    });

    connect(SRTMManager::get(), &SRTMManager::downloadProgress, this, [=](QString name, qint64 received, qint64 total) {
        if(!bars.contains(name)) {
            return;
        }
        bars[name]->setVisible(true);
        if(total > 0) {
            bars[name]->setRange(0, 1000);
            bars[name]->setValue(static_cast<int>(1000 * received / total));
            states[name]->setText(QString("%1 / %2 MB").arg(received / 1e6, 0, 'f', 1).arg(total / 1e6, 0, 'f', 1));
        } else {
            states[name]->setText(QString("%1 MB").arg(received / 1e6, 0, 'f', 1));
        }
    });
    connect(SRTMManager::get(), &SRTMManager::downloadFinished, this, [=](QString name, bool ok, QString error) {
        if(!bars.contains(name)) {
            return;
        }
        bars[name]->setVisible(false);
        if(ok) {
            states[name]->setText("downloaded");
            states[name]->setStyleSheet("color: #1a7f37;");
            checks[name]->setChecked(false);
            checks[name]->setEnabled(false);
        } else {
            // can be tried again
            states[name]->setText("failed");
            states[name]->setStyleSheet("color: #b42318;");
            states[name]->setToolTip(error);
            checks[name]->setEnabled(true);
        }
        update_button();
    });
}
