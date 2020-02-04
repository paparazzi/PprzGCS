#ifndef OSMTILEPROVIDER_H
#define OSMTILEPROVIDER_H

#include <tuple>
#include <map>
#include "tileitem.h"

typedef std::tuple<int, int, int> TileCoor;

class OSMTileProvider
{
public:
    OSMTileProvider();
    TileCoor tileCoorFromLatlon(double lat, double lon, int z);
    std::tuple<double, double> LatlonFromTile(int x, int y, int z);
    void fetch_tile(TileCoor t);

private:
    std::map<TileCoor, TileItem> tiles_map;
};

#endif // OSMTILEPROVIDER_H
