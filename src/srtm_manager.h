#ifndef SRTM_MANAGER_H
#define SRTM_MANAGER_H

#include <QString>
#include <map>
#include <optional>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QObject>
#include <QDialog>
#include <QKeyEvent>

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

    optional<int> get_elevation(double lat, double lon);
    void load_tiles(QList<QString> names);
    int load_tile(QString name, bool dl);
    QList<QString> get_tile_names(double _lat_min, double _lat_max, double _lon_min, double _lon_max);

private slots:
    void handleReply(QNetworkReply *reply);

private:
    static SRTMManager* singleton;
    explicit SRTMManager();

    QString get_tile_name(int lat, int lon);
    optional<QString> get_tile_region(QString name);

    static const QString srtm_url;

    map<QString, char*> tiles;   // key example: "N43E001"

    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;
    QList<QString> pending_downloads;
};


class SRTMDialog : public QDialog{
    Q_OBJECT
public:
    explicit SRTMDialog(QList<QString> tiles_names, QWidget *parent = nullptr);

signals:
    void tilesConfirmed(QList<QString>);

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

#endif // SRTM_MANAGER_H
