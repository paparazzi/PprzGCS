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

struct TileSourceConfig {
    char name[50];
    char dir[20];
    char addr[300];
    int posZoom;
    int posX;
    int posY;
    int zoomMin;
    int zoomMax;
    int tileSize;
};

enum TileSource {
    GOOGLE,
    OSM_CLASSIC,
    OSM_STAMEN,
    TERRAIN,
    HIKING,
    IGN,
    ICAO,
    FRANCE_DRONE_RESTRICTIONS
};

class TileProvider : public QObject
{
    Q_OBJECT
public:
    static const int TILE_SIZE = 256;
    static const int ZOOM_MIN = 0;
    static const int ZOOM_MAX = 19;

    explicit TileProvider(QObject *parent = nullptr);
    void fetch_tile(Point2DTile t, Point2DTile tObj);

    ///
    /// \brief getTile create tiles if they do not exist yet
    /// \return tile at the specified position
    ///
    TileItem* getTile(Point2DTile);

    int zoomLevel() {return _zoomLevel;}
    void setZoomLevel(int z);
    void setTileSource(TileSource s) {source = s;}

signals:
    // tileReady is the tile loaded in memory, tileObj is the one to actually display
    void displayTile(TileItem* tileReady, TileItem* tileObj);

private slots:
    void handleReply(QNetworkReply *reply);

private:

    int _zoomLevel;

    TileSource source;

    std::string tilePath(Point2DTile);
    QUrl tileUrl(Point2DTile);
    bool load_tile_from_disk(TileItem*);

    //QList<QMap<QString, TileItem*>> tiles_maps;
    TileItem* motherTile;
    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;

    QList<std::tuple<TileItem*, TileItem*>> downloading;
};

#endif // OSMTILEPROVIDER_H
