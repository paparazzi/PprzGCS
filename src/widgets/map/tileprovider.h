#ifndef OSMTILEPROVIDER_H
#define OSMTILEPROVIDER_H

#include <tuple>
#include <map>
#include "tileitem.h"
#include <QObject>
#include <QMap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include "point2dlatlon.h"

enum TileSource {
    GOOGLE,
    OSM_CLASSIC,
};

class TileProvider : public QObject
{
    Q_OBJECT
public:
    static const int TILE_SIZE = 256;
    static const int ZOOM_MIN = 0;
    static const int ZOOM_MAX = 19;

    explicit TileProvider(QObject *parent = nullptr);
    void fetch_tile(Point2DTile t);

    int zoomLevel() {return _zoomLevel;}
    void setZoomLevel(int z);
    void setTileSource(TileSource s) {source = s;}

signals:
    void tileReady(TileItem*, Point2DTile);

private slots:
    void handleReply(QNetworkReply *reply);

private:

    int _zoomLevel;

    TileSource source;

    std::string tilePath(Point2DTile);
    QUrl tileUrl(Point2DTile);
    void load_tile_from_disk(Point2DTile);

    QList<QMap<QString, TileItem*>> tiles_maps;
    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;

    QList<QString> downloading;
};

#endif // OSMTILEPROVIDER_H
