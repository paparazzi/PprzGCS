#include "osmtileprovider.h"
#include "math.h"


OSMTileProvider::OSMTileProvider()
{

}

TileCoor OSMTileProvider::tileCoorFromLatlon(double lat, double lon, int z)
{
    int tileX = static_cast<int>(floor((lon + 180.0) / 360.0 * (1 << z)));
    double latrad = lat * M_PI/180.0;
    int tileY = static_cast<int>(floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z)));
    return std::make_tuple(tileX, tileY, z);
}


std::tuple<double, double> OSMTileProvider::LatlonFromTile(int x, int y, int z)
{
    double lon = x / static_cast<double>(1 << z) * 360.0 - 180;
    double n = M_PI - 2.0 * M_PI * y / static_cast<double>(1 << z);
    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    return std::make_tuple(lat, lon);
}

void OSMTileProvider::fetch_tile(TileCoor t) {
    auto tile = tiles_map.find(t);
    if ( tile == tiles_map.end() ) {
      // not found
    } else {
      tile->second;
    }
}
