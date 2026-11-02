#ifndef SRTM_MANAGER_H
#define SRTM_MANAGER_H

#include <QString>
#include <map>
#include <optional>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QObject>
#include <QDialog>
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include "card_dialog.h"

using namespace std;

class SRTMManager : public PprzTool
{
    Q_OBJECT
public:
    explicit SRTMManager(PprzApplication* app, PprzToolbox* toolbox);
    ~SRTMManager();
    static SRTMManager* get() {
        return pprzApp()->toolbox()->srtmManager();
    }

    optional<int> get_elevation(double lat, double lon);
    void load_tiles(QList<QString> names, bool local_only=false);
    int load_tile(QString name, bool dl);
    QList<QString> get_tile_names(double _lat_min, double _lat_max, double _lon_min, double _lon_max);
    bool isDownloading(QString name) {return pending_downloads.contains(name);}

signals:
    void downloadProgress(QString name, qint64 received, qint64 total);
    void downloadFinished(QString name, bool ok, QString error);

private slots:
    void handleReply(QNetworkReply *reply);

private:
    QString get_tile_name(int lat, int lon);
    optional<QString> get_tile_region(QString name);

    static const QString srtm_url;

    map<QString, char*> tiles;   // key example: "N43E001"

    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;
    QList<QString> pending_downloads;
};


/**
 * The SRTM tiles for the flight plans and the map view: the ones on this computer, the missing
 * ones to download (checked), and the progress of each download.
 */
class SRTMDialog : public CardDialog {
    Q_OBJECT
public:
    explicit SRTMDialog(QList<QString> tiles_names, QList<QString> missing, QWidget *parent = nullptr);

signals:
    void tilesConfirmed(QList<QString>);
};

#endif // SRTM_MANAGER_H
