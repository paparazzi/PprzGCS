#ifndef OSMTILEPROVIDER_H
#define OSMTILEPROVIDER_H

#include <tuple>
#include <map>
#include "tileitem.h"
#include <QObject>
#include <QMap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>

typedef std::tuple<double, double, int> TileCoorD;
typedef std::tuple<int, int, int> TileCoorI;

#define COOR_I_OF_D(coorD) \
    std::make_tuple( \
        static_cast<int>(std::get<0>(coorD)), \
        static_cast<int>(std::get<1>(coorD)), \
        std::get<2>(coorD) \
    )

class OSMTileProvider : public QObject
{
    Q_OBJECT
public:

    static const int TILE_SIZE = 256;

    explicit OSMTileProvider(QObject *parent = nullptr);
    TileCoorD tileCoorFromLatlon(double lat, double lon, int z);
    std::tuple<double, double> LatlonFromTile(int x, int y, int z);
    void fetch_tile(TileCoorI t);

signals:
    void tileReady(TileItem*, TileCoorI);

private slots:
    void handleReply(QNetworkReply *reply);

private:

    std::string tilePath(TileCoorI);
    void load_tile_from_disk(TileCoorI);

    QMap<TileCoorI, TileItem*> tiles_map;
    QNetworkAccessManager* manager;
    QNetworkDiskCache* diskCache;

    QList<TileCoorI> downloading;
};

#endif // OSMTILEPROVIDER_H
