#ifndef SRTM_MANAGER_H
#define SRTM_MANAGER_H

#include <QString>
#include <map>
#include <optional>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QObject>

using namespace std;

class SRTMManager : public QObject
{
    Q_OBJECT
public:
    ~SRTMManager();
    static SRTMManager* get() {
        if(singleton == nullptr) {
            singleton = new SRTMManager();
        }
        return singleton;
    }

    void load_srtm(double _lat_min, double _lat_max, double _lon_min, double _lon_max);
    optional<int> get_elevation(double lat, double lon);

private slots:
    void handleReply(QNetworkReply *reply);

private:
    static SRTMManager* singleton;
    explicit SRTMManager();

    QString get_tile_name(int lat, int lon);
    void load_tile(QString name);
    optional<QString> get_tile_region(QString name);

    static const QString srtm_url;

    map<QString, char*> tiles;   // key example: "N43E001"

    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;
    QList<QString> pending_downloads;
};

#endif // SRTM_MANAGER_H
