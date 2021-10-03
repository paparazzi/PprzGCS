#ifndef POINT2DLATLON_H
#define POINT2DLATLON_H

#include "waypoint.h"
#include <algorithm>
#include "coordinate.h"
#include "proj.h"

class Point2DTile;
class CartesianCoor;

// TODO rename to EllipsoidalCoordiante or something like that
class Point2DLatLon: public Coordinate
{
public:
    Point2DLatLon();
    Point2DLatLon(double lat, double lon);
    //Point2DLatLon(Point2DTile pt);
    Point2DLatLon(Waypoint* wp);

    Point2DLatLon(PJ_COORD coord, QString crs);

    double lat() {return latp;}
    double lon() {return lonp;}
    void setLat(double lat) {latp = std::clamp(lat, -90., 90.);}
    void setLon(double lon) {lonp = std::clamp(lon, -180., 180.);}

    QString toString(bool sexagesimal=false);

    PJ_COORD toProj();

private:
    double latp;
    double lonp;
};

#endif // POINT2DLATLON_H
