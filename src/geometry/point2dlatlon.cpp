#include "point2dlatlon.h"
#include "point2dtile.h"
#include "math.h"

Point2DLatLon::Point2DLatLon(double lat, double lon, int zoom, int minZoom, int maxZoom) {
    zoomMinp = minZoom;
    zoomMaxp = maxZoom;
    setLat(lat);
    setLon(lon);
    setZoom(zoom);
}

Point2DLatLon::Point2DLatLon(Point2DTile pt) {
    double lon = pt.x() / static_cast<double>(1 << pt.zoom()) * 360.0 - 180;
    double n = M_PI - 2.0 * M_PI * pt.y() / static_cast<double>(1 << pt.zoom());
    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));

    setZoomMin(pt.zoomMin());
    setZoomMax(pt.zoomMax());

    setLat(lat);
    setLon(lon);
    setZoom(pt.zoom());

}
