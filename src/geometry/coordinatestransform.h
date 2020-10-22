#ifndef COORDINATESTRANSFORM_H
#define COORDINATESTRANSFORM_H

//#pragma once

#include <string>
#include <proj.h>
#include "point2dlatlon.h"
#include "point2dpseudomercator.h"
#include "point2dtile.h"
#include <QPointF>

using namespace std;

class CoordinatesTransform
{
public:
    enum Transform {
        NO_TRANSFORM,
        WGS84_UTM,
        WGS84_WEB_MERCATOR
    };

    static CoordinatesTransform* get() {
        if(singleton == nullptr) {
            singleton = new CoordinatesTransform();
        }
        return singleton;
    }


    ~CoordinatesTransform();

    bool isInitialized() {return transform != NO_TRANSFORM;}
    void init_WGS84_UTM(double lat, double lon);
    Point2DPseudoMercator WGS84_to_pseudoMercator(Point2DLatLon);
    Point2DLatLon pseudoMercator_to_WGS84(Point2DPseudoMercator);
    Point2DLatLon wgs84_from_scene(QPointF scenePoint, int zoom, int tile_size);

    void relative_to_wgs84(double lat0, double lon0, double x, double y, double* lat, double* lon);

    void distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut);


private:
    CoordinatesTransform();
    static CoordinatesTransform* singleton;

    static string utm_epsg(double lat, double lon);
    PJ_COORD trans(PJ_COORD src);
    PJ_COORD trans_inv(PJ_COORD src);
    double distance(Point2DLatLon pt1, Point2DLatLon pt2);
    double azimut(Point2DLatLon pt1, Point2DLatLon pt2);

    PJ_CONTEXT* pj_context;
    PJ* proj;
    PJ* proj_4326_3857;

    Transform transform;

};

#endif // COORDINATESTRANSFORM_H
