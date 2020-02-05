#ifndef POINT2DLATLON_H
#define POINT2DLATLON_H

#include "utils.h"

class Point2DTile;

class Point2DLatLon
{
public:
    Point2DLatLon(double lat, double lon, int zoom, int zoomMin=0, int zoomMax=19);
    Point2DLatLon(Point2DTile pt);

    double lat() {return latp;}
    double lon() {return lonp;}
    int zoom() {return zoomp;}
    void setLat(double lat) {latp = clamp(lat, -90., 90.);}
    void setLon(double lon) {lonp = clamp(lon, -180., 180.);}
    void setZoom(int zoom) { zoomp = clamp(zoom, zoomMinp, zoomMaxp); }

    int zoomMin() {return zoomMinp;}
    int zoomMax() {return zoomMaxp;}
    void setZoomMin(int zoomMin) {zoomMinp = zoomMin;}
    void setZoomMax(int zoomMax) {zoomMaxp = zoomMax;}

private:
    double latp;
    double lonp;
    int zoomp;

    int zoomMinp;
    int zoomMaxp;
};

#endif // POINT2DLATLON_H
