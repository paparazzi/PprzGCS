#ifndef COORDINATESTRANSFORM_H
#define COORDINATESTRANSFORM_H

//#pragma once

#include <string>
#include <proj.h>
#include "point2dlatlon.h"
#include "point2dpseudomercator.h"
#include "point2dtile.h"
#include <QPointF>
#include <QMap>
#include <QString>

class CoordinatesTransform
{
public:

    static inline const QString TRANSFORM_WGS84_WEB_MERCATOR = "WGS84_WEB_MERCATOR";
    static inline const QString TRANSFORM_NONE = "NO_TRANSFORM";

    static CoordinatesTransform* get() {
        if(singleton == nullptr) {
            singleton = new CoordinatesTransform();
        }
        return singleton;
    }


    ~CoordinatesTransform();

    Point2DPseudoMercator WGS84_to_pseudoMercator(Point2DLatLon);
    Point2DLatLon pseudoMercator_to_WGS84(Point2DPseudoMercator);
    Point2DLatLon wgs84_from_scene(QPointF scenePoint, int zoom, int tile_size);

    Point2DLatLon relative_utm_to_wgs84(Point2DLatLon origin, double x, double y);
    void wgs84_to_relative_utm(Point2DLatLon origin, Point2DLatLon geo, double& x, double& y);
    Point2DLatLon ltp_to_wgs84(Point2DLatLon origin, double x, double y);
    void wgs84_to_ltp(Point2DLatLon origin, Point2DLatLon geo, double& x, double& y);

    void distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut);


private:
    CoordinatesTransform();
    static CoordinatesTransform* singleton;

    static QString utm_epsg(double lat, double lon);

    PJ_CONTEXT* pj_context;
    QMap<QString, PJ*> projectors;

};

#endif // COORDINATESTRANSFORM_H
