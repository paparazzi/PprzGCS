#ifndef COORDINATESTRANSFORM_H
#define COORDINATESTRANSFORM_H

#include <string>
#include <proj.h>
#include "point2dlatlon.h"

using namespace std;

class CoordinatesTransform
{
public:
    enum Transform {
        NO_TRANSFORM,
        WGS84_UTM,
        WGS84_WEB_MERCATOR
    };

    CoordinatesTransform();
    ~CoordinatesTransform();

    bool isInitialized() {return transform != NO_TRANSFORM;}
    void init_WGS84_UTM(double lat, double lon);
    void init_WGS84_web_mercator();
    PJ_COORD trans(PJ_COORD src);
    PJ_COORD trans_inv(PJ_COORD src);
    void relative_to_wgs84(double lat0, double lon0, double x, double y, double* lat, double* lon);
    double distance(Point2DLatLon pt1, Point2DLatLon pt2);
    double azimut(Point2DLatLon pt1, Point2DLatLon pt2);
    void distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut);


private:
    static string utm_epsg(double lat, double lon);
    PJ_CONTEXT* pj_context;
    PJ* proj;

    Transform transform;

};

#endif // COORDINATESTRANSFORM_H
