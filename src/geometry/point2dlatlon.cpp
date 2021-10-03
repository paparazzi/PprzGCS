#include "point2dlatlon.h"
#include "point2dtile.h"
#include "math.h"
#include "maputils.h"
#include "cartesian_coor.h"

#include "iostream"

Point2DLatLon::Point2DLatLon() : Coordinate("EPSG:4326"),
    latp(0), lonp(0)
{
}

Point2DLatLon::Point2DLatLon(double lat, double lon): Point2DLatLon()
{
    setLat(lat);
    setLon(lon);
}

//Point2DLatLon::Point2DLatLon(Point2DTile pt) {
//    double lon = pt.x() / static_cast<double>(1 << pt.zoom()) * 360.0 - 180;
//    double n = M_PI - 2.0 * M_PI * pt.y() / static_cast<double>(1 << pt.zoom());
//    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
//    setLat(lat);
//    setLon(lon);
//}

Point2DLatLon::Point2DLatLon(Waypoint* wp) : Point2DLatLon()
{
    latp = wp->getLat();
    lonp = wp->getLon();
}

Point2DLatLon::Point2DLatLon(PJ_COORD coord, QString crs) : Coordinate(crs),
    latp(coord.lp.lam), lonp(coord.lp.phi)
{
}

QString Point2DLatLon::toString(bool sexagesimal) {
    if(sexagesimal) {
        auto sexformat = [](double nb) {
            int deg = static_cast<int>(nb);
            int min = static_cast<int>((nb - deg) * 60);
            int sec = static_cast<int>((((nb - deg) * 60) - min)*60);
            QString txt = QString("%1").arg(deg, 3, 10, QChar(' ')) + "° " + QString("%1").arg(min, 2, 10, QChar('0')) + "' " + QString("%1").arg(sec, 2, 10, QChar('0')) + "\"";
            return txt;
        };

        QString txtLat = sexformat(abs(latp));
        QString txtLon = sexformat(abs(lonp));

        QString latGeo = latp > 0 ? "N" : "S";
        QString lonGeo = lonp > 0 ? "E" : "W";

        QString txt = txtLat + latGeo + " " + txtLon + lonGeo;
        return txt;
    } else {
        // Why 6 digits ? Its smaller than a person, and bigger than a waldo: https://xkcd.com/2170/
        QString txt = QString("%1").arg(latp, 10, 'f', 6, QChar(' ')) + "," + QString("%1").arg(lonp, 11, 'f', 6, QChar(' '));
        return txt;
    }
}

PJ_COORD Point2DLatLon::toProj() {
    return proj_coord(latp, lonp, 0, 0);
}
