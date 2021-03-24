#include "coordinatestransform.h"
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <mutex>

namespace { std::recursive_mutex mtx; }

CoordinatesTransform* CoordinatesTransform::singleton = nullptr;

CoordinatesTransform::CoordinatesTransform() : pj_context(nullptr)
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
    for(auto proj: projectors) {
        proj_destroy(proj);
    }
    proj_context_destroy(pj_context);
}

Point2DPseudoMercator CoordinatesTransform::WGS84_to_pseudoMercator(Point2DLatLon ll) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    auto plop = proj_trans (projectors[TRANSFORM_WGS84_WEB_MERCATOR], PJ_FWD, proj_coord(ll.lat(), ll.lon(), 0, 0));
    return Point2DPseudoMercator(plop.xy.x, plop.xy.y);;
}

Point2DLatLon CoordinatesTransform::pseudoMercator_to_WGS84(Point2DPseudoMercator pm) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    auto pt_latlon = proj_trans (projectors[TRANSFORM_WGS84_WEB_MERCATOR], PJ_INV, proj_coord(pm.x(), pm.y(), 0, 0));
    return Point2DLatLon(pt_latlon.lp.lam, pt_latlon.lp.phi);
}

Point2DLatLon CoordinatesTransform::wgs84_from_scene(QPointF scenePoint, int zoom, int tile_size) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);
    auto pi_tile = Point2DTile(scenePoint.x()/tile_size, scenePoint.y()/tile_size, zoom);
    auto pt_ps_merc = Point2DPseudoMercator(pi_tile);
    auto pt_latlon = proj_trans (projectors[TRANSFORM_WGS84_WEB_MERCATOR], PJ_INV, proj_coord(pt_ps_merc.x(), pt_ps_merc.y(), 0, 0));
    return Point2DLatLon(pt_latlon.lp.lam, pt_latlon.lp.phi);
}


Point2DLatLon CoordinatesTransform::relative_ltp_to_wgs84(Point2DLatLon origin, double x, double y) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    auto pm_orig = WGS84_to_pseudoMercator(origin);
    Point2DPseudoMercator pm(pm_orig.x() + x, pm_orig.y() + y);
    auto geo = pseudoMercator_to_WGS84(pm);

    return geo;
}

Point2DLatLon CoordinatesTransform::relative_utm_to_wgs84(Point2DLatLon origin, double x, double y) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    auto epsg = utm_epsg(origin.lat(), origin.lon());
    QString proj_name = "EPSG:4326_" + epsg;
    if(!projectors.contains(proj_name)) {
        auto proj = proj_create_crs_to_crs (pj_context, "EPSG:4326", epsg.toStdString().c_str(), nullptr);
        projectors[proj_name] = proj;
    }

    auto coord = proj_coord (origin.lat(), origin.lon(), 0, 0);
    PJ_COORD pos_utm = proj_trans (projectors[proj_name], PJ_INV, coord);
    pos_utm.xy.x += x;
    pos_utm.xy.y += y;
    PJ_COORD pos_latlon = proj_trans (projectors[proj_name], PJ_FWD, pos_utm);

    return Point2DLatLon(pos_latlon.lp.lam, pos_latlon.lp.phi);
}


void CoordinatesTransform::distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut) {
    const std::lock_guard<std::recursive_mutex> lock(mtx);

    auto epsg = utm_epsg(pt1.lat(), pt1.lon());
    QString proj_name = "EPSG:4326_" + epsg;

    auto geo1 = proj_coord (pt1.lat(), pt1.lon(), 0, 0);
    auto geo2 = proj_coord (pt2.lat(), pt2.lon(), 0, 0);

    PJ_COORD utm1 = proj_trans (projectors[proj_name], PJ_FWD, geo1);
    PJ_COORD utm2 = proj_trans (projectors[proj_name], PJ_FWD, geo2);

    distance = proj_xy_dist(utm1, utm2);
    azimut = atan2(utm2.xy.x-utm1.xy.x, utm2.xy.y-utm1.xy.y) * 180.0 / M_PI;
    if(azimut < 0) {
        azimut += 360.0;
    }
}

//////////// private functions ////////////

QString CoordinatesTransform::utm_epsg(double lat, double lon) {
    QString epsg("EPSG:32");
    if(lat > 0) {
        epsg += "6";
    } else {
        epsg += "7";
    }
    int zone_nb = (static_cast<int>(lon) + 180)/6 + 1;
    epsg += QString("%1").arg(zone_nb, 2, 10, QChar('0'));
    return epsg;
}
