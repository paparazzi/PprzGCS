#include "coordinatestransform.h"
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <math.h>

CoordinatesTransform::CoordinatesTransform() : pj_context(nullptr), proj(nullptr), transform(NO_TRANSFORM)
{
    pj_context = proj_context_create();
}

CoordinatesTransform::~CoordinatesTransform()
{
    proj_destroy(proj);
    proj_context_destroy(pj_context);
}

void CoordinatesTransform::init_WGS84_UTM(double lat, double lon) {
    string epsg = utm_epsg(lat, lon);
    proj = proj_create_crs_to_crs (pj_context,
        "EPSG:4326",
        epsg.c_str(),
        nullptr);
    transform = WGS84_UTM;
}

void CoordinatesTransform::init_WGS84_web_mercator() {
    proj = proj_create_crs_to_crs (pj_context,
        "EPSG:4326",
        "EPSG:3857",
        nullptr);
    transform = WGS84_WEB_MERCATOR;
}


void CoordinatesTransform::relative_to_wgs84(double lat0, double lon0, double x, double y, double* lat, double* lon) {
    assert(transform == WGS84_UTM);

    PJ_COORD pos_utm = trans_inv(proj_coord (lat0, lon0, 0, 0));
    pos_utm.xy.x += x;
    pos_utm.xy.y += y;
    PJ_COORD pos_latlon = trans(pos_utm);
    *lat = pos_latlon.lp.lam;
    *lon = pos_latlon.lp.phi;
}

double CoordinatesTransform::distance(Point2DLatLon pt1, Point2DLatLon pt2) {
    PJ_COORD a = trans(proj_coord (pt1.lat(), pt1.lon(), 0, 0));
    PJ_COORD b = trans(proj_coord (pt2.lat(), pt2.lon(), 0, 0));

    double dist = proj_xy_dist(a, b);
    return dist;
}

double CoordinatesTransform::azimut(Point2DLatLon pt1, Point2DLatLon pt2) {
    PJ_COORD a = trans(proj_coord (pt1.lat(), pt1.lon(), 0, 0));
    PJ_COORD b = trans(proj_coord (pt2.lat(), pt2.lon(), 0, 0));

    double az = atan2(b.xy.x-a.xy.x, b.xy.y-a.xy.y) * 180.0 / M_PI;
    if(az < 0) {
        az += 360.0;
    }
    return az;
}

void CoordinatesTransform::distance_azimut(Point2DLatLon pt1, Point2DLatLon pt2, double& distance, double& azimut) {
    PJ_COORD a = trans(proj_coord (pt1.lat(), pt1.lon(), 0, 0));
    PJ_COORD b = trans(proj_coord (pt2.lat(), pt2.lon(), 0, 0));

    distance = proj_xy_dist(a, b);
    azimut = atan2(b.xy.x-a.xy.x, b.xy.y-a.xy.y) * 180.0 / M_PI;
    if(azimut < 0) {
        azimut += 360.0;
    }
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
