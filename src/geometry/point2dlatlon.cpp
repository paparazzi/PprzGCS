#include "point2dlatlon.h"
#include "point2dtile.h"
#include "math.h"
#include "maputils.h"

#include "iostream"

Point2DLatLon::Point2DLatLon(double lat, double lon) {
    setLat(lat);
    setLon(lon);
}

Point2DLatLon::Point2DLatLon(Point2DTile pt) {
    double lon = pt.x() / static_cast<double>(1 << pt.zoom()) * 360.0 - 180;
    double n = M_PI - 2.0 * M_PI * pt.y() / static_cast<double>(1 << pt.zoom());
    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    setLat(lat);
    setLon(lon);
}

Point2DLatLon::Point2DLatLon(Waypoint& wp) {
    latp = wp.getLat();
    lonp = wp.getLon();
}

Point2DLatLon::Point2DLatLon(shared_ptr<Waypoint> wp) {
    latp = wp->getLat();
    lonp = wp->getLon();
}
