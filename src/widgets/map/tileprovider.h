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
#include "point2dtile.h"
#include "tileproviderconfig.h"
#include <memory>


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
    explicit TileProvider(std::unique_ptr<TileProviderConfig>& config, int z = 0, int tileDisplaySize = 0, QObject *parent = nullptr);
    void fetch_tile(Point2DTile t, Point2DTile tObj);

    ///
    /// \brief getTile create tiles if they do not exist yet
    /// \return tile at the specified position
    ///
    TileItem* getTile(Point2DTile);

    int zoomLevel() {return _zoomLevel;}
    void setZoomLevel(int z);
    int zValue() {return z_value;}
    void setZValue(int z);

    std::unique_ptr<TileProviderConfig>& getConfig() {return config;}

    Point2DTile tilePoint(QPointF scenePos);

signals:
    // tileReady is the tile loaded in memory, tileObj is the one to actually display
    void displayTile(TileItem* tileReady, TileItem* tileObj);

private slots:
    void handleReply(QNetworkReply *reply);

private:

    std::unique_ptr<TileProviderConfig>& config;
    int _zoomLevel;
    int z_value;

    /// All displayed tiles must have the same size across tilesProviders
    /// for the coordinates to be aligned
    int tileDisplaySize;

    std::string tilePath(Point2DTile);
    QUrl tileUrl(Point2DTile);
    bool load_tile_from_disk(TileItem*);

    //QList<QMap<QString, TileItem*>> tiles_maps;
    TileItem* motherTile;
    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;

    //QList<std::tuple<TileItem*, TileItem*>> downloading;
};

#endif // OSMTILEPROVIDER_H
