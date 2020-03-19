#ifndef POINT2DLATLON_H
#define POINT2DLATLON_H

#include "waypoint.h"
#include <algorithm>

class Point2DTile;

class Point2DLatLon
{
public:
    Point2DLatLon(double lat, double lon);
    Point2DLatLon(Point2DTile pt);
    Point2DLatLon(Waypoint& wp);
    Point2DLatLon(shared_ptr<Waypoint> wp);

    double lat() {return latp;}
    double lon() {return lonp;}
    void setLat(double lat) {latp = clamp(lat, -90., 90.);}
    void setLon(double lon) {lonp = clamp(lon, -180., 180.);}

private:
    double latp;
    double lonp;
};

#endif // POINT2DLATLON_H
