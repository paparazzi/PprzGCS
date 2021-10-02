#include "coordinatestransform.h"
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <mutex>
#include <QDebug>
#include "PprzToolbox.h"
#include "PprzApplication.h"
#include "cartesian_coor.h"

namespace { std::recursive_mutex mtx; }

CoordinatesTransform::CoordinatesTransform(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox),
    pj_context(nullptr)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    pj_context = proj_context_create();
    auto proj_4326_3857 = proj_create_crs_to_crs (pj_context,
        "EPSG:4326",
        "EPSG:3857",
        nullptr);
    projectors[TRANSFORM_WGS84_WEB_MERCATOR] = proj_4326_3857;
}

CoordinatesTransform::~CoordinatesTransform()
{
    for(auto proj: qAsConst(projectors)) {
        proj_destroy(proj);
    }
    proj_context_destroy(pj_context);
}

Point2DPseudoMercator CoordinatesTransform::to_pseudoMercator(GeographicCoordinate& input) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    QString in_crs = input.getCRS();
    QString out_crs = "EPSG:3857";
    auto proj = crs_to_crs(in_crs, out_crs);

    auto in_coor = input.toProj();
    PJ_COORD out = proj_trans (proj, PJ_FWD, in_coor);

    return Point2DPseudoMercator(out);
}

Point2DLatLon CoordinatesTransform::to_WGS84(CartesianCoor car) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    QString in_crs = car.getCRS();
    QString out_crs = "EPSG:4326";
    auto proj = crs_to_crs(in_crs, out_crs);

    auto pt_latlon = proj_trans (proj, PJ_FWD, car.toProj());
    return Point2DLatLon(pt_latlon, out_crs);
}

PJ* CoordinatesTransform::crs_to_crs(QString in_crs, QString out_crs) {
    QString proj_name = QString("%1_%2").arg(in_crs, out_crs);
    if(!projectors.contains(proj_name)) {
        auto proj = proj_create_crs_to_crs (pj_context, in_crs.toStdString().c_str(), out_crs.toStdString().c_str(), nullptr);
        projectors[proj_name] = proj;
    }
    return projectors[proj_name];
}

Point2DLatLon CoordinatesTransform::wgs84_from_scene(QPointF scenePoint, int zoom, int tile_size) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    auto pi_tile = Point2DTile(scenePoint.x()/tile_size, scenePoint.y()/tile_size, zoom);
    auto pt_ps_merc = Point2DPseudoMercator(pi_tile);
    auto pt_latlon = proj_trans (projectors[TRANSFORM_WGS84_WEB_MERCATOR], PJ_INV, proj_coord(pt_ps_merc.x(), pt_ps_merc.y(), 0, 0));
    return Point2DLatLon(pt_latlon.lp.lam, pt_latlon.lp.phi);
}


Point2DLatLon CoordinatesTransform::ltp_to_wgs84(Point2DLatLon origin, double x, double y) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    // cf https://proj.org/operations/projections/ortho.html
    // at least 6 digits are needed to have a good precision.
    // The default is too much rounded, which give a finale error of 1m...    

    QString in_crs = QString("+proj=ortho +lat_0=%1 +lon_0=%2").arg(origin.lat(), 0, 'f', 7).arg(origin.lon(),  0, 'f', 7);
    QString out_crs = "EPSG:4326";
    auto proj = crs_to_crs(in_crs, out_crs);

    auto enu = proj_coord (x, y, 0, 0);
    PJ_COORD geo = proj_trans (proj, PJ_FWD, enu);

    return Point2DLatLon(geo, out_crs);
}

void CoordinatesTransform::wgs84_to_ltp(Point2DLatLon origin, Point2DLatLon geo, double& x, double& y) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    QString in_crs = "EPSG:4326";
    QString out_crs = QString("+proj=ortho +lat_0=%1 +lon_0=%2").arg(origin.lat()).arg(origin.lon());
    auto proj = crs_to_crs(in_crs, out_crs);

    auto coord = proj_coord(geo.lat(), geo.lon(), 0, 0);
    PJ_COORD enu = proj_trans (proj, PJ_FWD, coord);

    x = enu.enu.e;
    y = enu.enu.n;
}

Point2DLatLon CoordinatesTransform::relative_utm_to_wgs84(Point2DLatLon origin, double x, double y) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    QString in_crs = "EPSG:4326";
    QString out_crs = CartesianCoor::utm_epsg(origin.lat(), origin.lon());
    auto proj = crs_to_crs(in_crs, out_crs);

    auto coord = proj_coord (origin.lat(), origin.lon(), 0, 0);
    PJ_COORD pos_utm = proj_trans (proj, PJ_FWD, coord);
    pos_utm.xy.x += x;
    pos_utm.xy.y += y;
    PJ_COORD pos_latlon = proj_trans (proj, PJ_INV, pos_utm);

    return Point2DLatLon(pos_latlon, "EPSG:4326");
}

Point2DLatLon CoordinatesTransform::utm_to_wgs84(double east, double north, int zone, bool isNorth) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    QString in_crs = CartesianCoor::utm_epsg(zone, isNorth);
    QString out_crs = "EPSG:4326";
    auto proj = crs_to_crs(in_crs, out_crs);

    auto utm_coord = proj_coord (east, north, 0, 0);
    PJ_COORD pos_latlon = proj_trans (proj, PJ_FWD, utm_coord);

    return Point2DLatLon(pos_latlon, out_crs);
}

void CoordinatesTransform::wgs84_to_relative_utm(Point2DLatLon origin, Point2DLatLon geo, double& x, double& y) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    QString in_crs = "EPSG:4326";
    QString out_crs = CartesianCoor::utm_epsg(origin.lat(), origin.lon());
    auto proj = crs_to_crs(in_crs, out_crs);

    auto coord_orig = proj_coord (origin.lat(), origin.lon(), 0, 0);
    auto coord_geo = proj_coord (geo.lat(), geo.lon(), 0, 0);

    PJ_COORD orig_utm = proj_trans (proj, PJ_FWD, coord_orig);
    PJ_COORD pos_utm = proj_trans (proj, PJ_FWD, coord_geo);

    x = pos_utm.xy.x - orig_utm.xy.x;
    y = pos_utm.xy.y - orig_utm.xy.y;

}

void CoordinatesTransform::distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    QString in_crs = "EPSG:4326";
    QString out_crs = CartesianCoor::utm_epsg(pt1.lat(), pt1.lon());
    auto proj = crs_to_crs(in_crs, out_crs);

    auto geo1 = proj_coord (pt1.lat(), pt1.lon(), 0, 0);
    auto geo2 = proj_coord (pt2.lat(), pt2.lon(), 0, 0);

    PJ_COORD utm1 = proj_trans (proj, PJ_FWD, geo1);
    PJ_COORD utm2 = proj_trans (proj, PJ_FWD, geo2);

    distance = proj_xy_dist(utm1, utm2);

    azimut = qRadiansToDegrees(atan2(utm2.xy.x-utm1.xy.x, utm2.xy.y-utm1.xy.y));
    if(azimut < 0) {
        azimut += 360.0;
    }
}
