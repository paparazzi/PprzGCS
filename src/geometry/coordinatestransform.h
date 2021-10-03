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
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include "coordinate.h"

class CoordinatesTransform : public PprzTool
{
public:

    CoordinatesTransform(PprzApplication* app, PprzToolbox* toolbox);

    static inline const QString TRANSFORM_WGS84_WEB_MERCATOR = "WGS84_WEB_MERCATOR";
    static inline const QString TRANSFORM_NONE = "NO_TRANSFORM";

    static CoordinatesTransform* get() {
        return pprzApp()->toolbox()->coordinatesTransform();
    }


    ~CoordinatesTransform();

    PJ* crs_to_crs(QString in_crs, QString out_crs);

    Point2DPseudoMercator WGS84_to_pseudoMercator(Point2DLatLon);
    Point2DPseudoMercator to_pseudoMercator(Coordinate& input);
    Point2DLatLon to_WGS84(CartesianCoor);

    Point2DLatLon relative_utm_to_wgs84(Point2DLatLon origin, double x, double y);
    void wgs84_to_relative_utm(Point2DLatLon origin, Point2DLatLon geo, double& x, double& y);
    Point2DLatLon ltp_to_wgs84(Point2DLatLon origin, double x, double y);
    void wgs84_to_ltp(Point2DLatLon origin, Point2DLatLon geo, double& x, double& y);
    Point2DLatLon utm_to_wgs84(double east, double north, int zone, bool isNorth);

    void distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut);


private:

    PJ_CONTEXT* pj_context;
    QMap<QString, PJ*> projectors;

};

#endif // COORDINATESTRANSFORM_H
