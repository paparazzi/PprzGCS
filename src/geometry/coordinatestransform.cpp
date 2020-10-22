#include "coordinatestransform.h"
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <mutex>

namespace { std::mutex mtx; }

CoordinatesTransform* CoordinatesTransform::singleton = nullptr;

CoordinatesTransform::CoordinatesTransform() : pj_context(nullptr), proj(nullptr), transform(NO_TRANSFORM)
{
    const std::lock_guard<std::mutex> lock(mtx);
    pj_context = proj_context_create();
    proj_4326_3857 = proj_create_crs_to_crs (pj_context,
        "EPSG:4326",
        "EPSG:3857",
        nullptr);
}

CoordinatesTransform::~CoordinatesTransform()
{
    proj_destroy(proj);
    proj_context_destroy(pj_context);
}

void CoordinatesTransform::init_WGS84_UTM(double lat, double lon) {
    const std::lock_guard<std::mutex> lock(mtx);
    string epsg = utm_epsg(lat, lon);
    if(proj != nullptr) {
        proj_destroy(proj);
    }
    proj = proj_create_crs_to_crs (pj_context,
        "EPSG:4326",
        epsg.c_str(),
        nullptr);
    transform = WGS84_UTM;
}

Point2DPseudoMercator CoordinatesTransform::WGS84_to_pseudoMercator(Point2DLatLon ll) {
    const std::lock_guard<std::mutex> lock(mtx);
    auto plop = proj_trans (proj_4326_3857, PJ_FWD, proj_coord(ll.lat(), ll.lon(), 0, 0));
    return Point2DPseudoMercator(plop.xy.x, plop.xy.y);;
}

Point2DLatLon CoordinatesTransform::pseudoMercator_to_WGS84(Point2DPseudoMercator pm) {
    const std::lock_guard<std::mutex> lock(mtx);
    auto pt_latlon = proj_trans (proj_4326_3857, PJ_INV, proj_coord(pm.x(), pm.y(), 0, 0));
    return Point2DLatLon(pt_latlon.lp.lam, pt_latlon.lp.phi);
}

Point2DLatLon CoordinatesTransform::wgs84_from_scene(QPointF scenePoint, int zoom, int tile_size) {
    const std::lock_guard<std::mutex> lock(mtx);
    auto pi_tile = Point2DTile(scenePoint.x()/tile_size, scenePoint.y()/tile_size, zoom);
    auto pt_ps_merc = Point2DPseudoMercator(pi_tile);
    auto pt_latlon = proj_trans (proj_4326_3857, PJ_INV, proj_coord(pt_ps_merc.x(), pt_ps_merc.y(), 0, 0));
    return Point2DLatLon(pt_latlon.lp.lam, pt_latlon.lp.phi);
}


void CoordinatesTransform::relative_to_wgs84(double lat0, double lon0, double x, double y, double* lat, double* lon) {
    const std::lock_guard<std::mutex> lock(mtx);
    assert(transform == WGS84_UTM);
    PJ_COORD pos_utm = trans_inv(proj_coord (lat0, lon0, 0, 0));
    pos_utm.xy.x += x;
    pos_utm.xy.y += y;
    PJ_COORD pos_latlon = trans(pos_utm);
    *lat = pos_latlon.lp.lam;
    *lon = pos_latlon.lp.phi;
}


void CoordinatesTransform::distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut) {
    const std::lock_guard<std::mutex> lock(mtx);
    PJ_COORD a = trans(proj_coord (pt1.lat(), pt1.lon(), 0, 0));
    PJ_COORD b = trans(proj_coord (pt2.lat(), pt2.lon(), 0, 0));

    distance = proj_xy_dist(a, b);
    azimut = atan2(b.xy.x-a.xy.x, b.xy.y-a.xy.y) * 180.0 / M_PI;
    if(azimut < 0) {
        azimut += 360.0;
    }
}

//////////// private functions ////////////

double CoordinatesTransform::distance(Point2DLatLon pt1, Point2DLatLon pt2) {
    const std::lock_guard<std::mutex> lock(mtx);
    PJ_COORD a = trans(proj_coord (pt1.lat(), pt1.lon(), 0, 0));
    PJ_COORD b = trans(proj_coord (pt2.lat(), pt2.lon(), 0, 0));

    double dist = proj_xy_dist(a, b);
    return dist;
}

double CoordinatesTransform::azimut(Point2DLatLon pt1, Point2DLatLon pt2) {
    const std::lock_guard<std::mutex> lock(mtx);
    PJ_COORD a = trans(proj_coord (pt1.lat(), pt1.lon(), 0, 0));
    PJ_COORD b = trans(proj_coord (pt2.lat(), pt2.lon(), 0, 0));

    double az = atan2(b.xy.x-a.xy.x, b.xy.y-a.xy.y) * 180.0 / M_PI;
    if(az < 0) {
        az += 360.0;
    }
    return az;
}

PJ_COORD CoordinatesTransform::trans(PJ_COORD src) {
    return proj_trans (proj, PJ_FWD, src);
}

PJ_COORD CoordinatesTransform::trans_inv(PJ_COORD src) {
    return proj_trans (proj, PJ_INV, src);
}

string CoordinatesTransform::utm_epsg(double lat, double lon) {
    stringstream ss;
    ss << "EPSG:32";
    if(lat > 0) {
        ss << "6";
    } else {
        ss << "7";
    }
    int zone_nb = (static_cast<int>(lon) + 180)/6 + 1;
    ss << setw(2) << setfill('0') << zone_nb;
    std::string epsg = ss.str();
    return epsg;
}
