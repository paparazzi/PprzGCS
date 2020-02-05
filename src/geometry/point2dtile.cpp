#include "point2dtile.h"
#include "point2dlatlon.h"
#include "math.h"

Point2DTile::Point2DTile(double x, double y, int zoom, int minZoom, int maxZoom) {
    zoomMinp = minZoom;
    zoomMaxp = maxZoom;
    setX(x);
    setY(y);
    setZoom(zoom);
}

Point2DTile::Point2DTile(Point2DLatLon latLon) {
    double tileX = (latLon.lon() + 180.0) / 360.0 * (1 << latLon.zoom());
    double latrad = latLon.lat() * M_PI/180.0;
    double tileY = (1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << latLon.zoom());
    setZoomMin(latLon.zoomMin());
    setZoomMax(latLon.zoomMax());
    setX(tileX);
    setY(tileY);
    setZoom(latLon.zoom());

}

QString Point2DTile::to_string() {
    std::string pStr;
    pStr += "X" + std::to_string(xp) + "Y" + std::to_string(yp) + "Z" + std::to_string(zoomp);
    return QString(pStr.c_str());
}
