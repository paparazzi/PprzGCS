#include "waypoint.h"
#include <math.h>
#include <iostream>
#include "coordinatestransform.h"
#include "gcs_utils.h"
#include <QDebug>
#include "flightplan.h"

Waypoint::Waypoint(Waypoint* original, QObject* parent):
    QObject(parent)
{
    type = original->type;
    id = original->id;
    lat = original->lat;
    lon = original->lon;
    origin = original->getOrigin();
    alt = original->alt;
    alt_type = original->alt_type;
    name = original->name;
    xml_attibutes = original->xml_attibutes;
    affectFlightPlan();
}

Waypoint::Waypoint(QString name, uint8_t id, QObject* parent) :
    QObject(parent),
    type(WGS84), id(id), lat(0), lon(0), origin(nullptr), alt(0), name(name)
{
    affectFlightPlan();
}

Waypoint::Waypoint(QString name, uint8_t id, Point2DLatLon pos, double alt, QObject* parent):
    Waypoint(name, id, pos.lat(), pos.lon(), alt, parent)
{}

Waypoint::Waypoint(QString name, uint8_t id, double lat, double lon, double alt, QObject* parent):
    Waypoint(name, id, parent)
{
    type = WGS84;
    this->lat = lat;
    this->lon = lon;
    this->alt = alt;
    alt_type = ALT;
}



Waypoint::Waypoint(QDomElement wp, uint8_t wp_id, Waypoint* orig, double defaultAlt, WpFrame frame_type, QObject* parent):
    QObject(parent),
    type(frame_type), origin(orig)
{
    Waypoint::WpAltType altType = Waypoint::WpAltType::ALT;
    double alt;
    if(wp.hasAttribute("height")) {
        altType = Waypoint::WpAltType::HEIGHT;
        alt = wp.attribute("height").toDouble();
    } else if(wp.hasAttribute("alt")) {
        alt = wp.attribute("alt").toDouble();
    } else {
        alt = defaultAlt;
    }

    name = wp.attribute("name");
    id = wp_id;

    if(wp.hasAttribute("lat") && wp.hasAttribute("lon")) {
        type = WGS84;
        this->lat = parse_coordinate(wp.attribute("lat"));
        this->lon = parse_coordinate(wp.attribute("lon"));
        this->alt = alt;
        alt_type = ALT;

        if(altType == Waypoint::WpAltType::HEIGHT) {
            throw std::runtime_error("Unimplemented! Can't (yet) create absolute waypoint with relative height!");
        }
    }
    else if(wp.hasAttribute("x") && wp.hasAttribute("y")) {
        auto x = wp.attribute("x").toDouble();
        auto y = wp.attribute("y").toDouble();
        if(frame_type == WpFrame::UTM) {
            auto latlon = CoordinatesTransform::get()->relative_utm_to_wgs84(orig,  x, y);
            this->lat = latlon.lat();
            this->lon = latlon.lon();
        } else if(frame_type == WpFrame::LTP) {
            auto latlon = CoordinatesTransform::get()->ltp_to_wgs84(orig, x, y);
            this->lat = latlon.lat();
            this->lon = latlon.lon();
        }
        this->alt = alt;
        this->alt_type = altType;
    } else {
        throw std::runtime_error("You must specify either x/y or lat/lon!");
    }

    auto attr = wp.attributes();
    for(int i=0; i < attr.count(); i++) {
        auto att = attr.item(i).toAttr();
       xml_attibutes[att.name()] = att.value();
    }
    affectFlightPlan();
}

void Waypoint::affectFlightPlan() {
    auto fp = dynamic_cast<FlightPlan*>(parent());
    if(fp) {
        flight_plan = fp;
    }
}

void Waypoint::setName(QString new_name) {
    name = new_name;
}


double Waypoint::getLon() const {
    return lon;
}

double Waypoint::getLat() const {
    return lat;
}

void Waypoint::setLat(double lat) {
        this->lat = lat;

}

void Waypoint::setLon(double lon) {
    this->lon = lon;
}

/**
 * @brief Waypoint::setRelative
 * @param frame: LTP/UTM(/WGS84)
 * @param dx: dx from wp, in frame whose origin is frame_orig
 * @param dy: dy from wp, in frame whose origin is frame_orig
 * @param wp: from where dx and dy are expressed. Default to origin if null.
 */
void Waypoint::setRelative(WpFrame frame, double dx, double dy, Waypoint* wp) {
    if(wp == nullptr) {
        wp = origin;
    }
    double x0, y0;
    Point2DLatLon geo(0, 0);
    if(frame == Waypoint::LTP) {
        CoordinatesTransform::get()->wgs84_to_ltp(origin, wp, x0, y0);
        double x = x0 + dx;
        double y = y0 + dy;
        geo = CoordinatesTransform::get()->ltp_to_wgs84(origin, x, y);
    } else {
        CoordinatesTransform::get()->wgs84_to_relative_utm(origin, wp, x0, y0);
        double x = x0 + dx;
        double y = y0 + dy;
        geo = CoordinatesTransform::get()->relative_utm_to_wgs84(origin, x, y);
    }
    setLat(geo.lat());
    setLon(geo.lon());
}

void Waypoint::getRelative(WpFrame frame, double &dx, double &dy, Waypoint* wp) {
    if(wp == nullptr) {
        wp = origin;
    }
    double wp_x, wp_y, x, y;
    if(frame == Waypoint::LTP) {
        CoordinatesTransform::get()->wgs84_to_ltp(origin, wp, wp_x, wp_y);    // coordinates of wp/origin
        CoordinatesTransform::get()->wgs84_to_ltp(origin, this, x, y);        // coordinates of this/origin
    } else {
        CoordinatesTransform::get()->wgs84_to_relative_utm(origin, wp, wp_x, wp_y);
        CoordinatesTransform::get()->wgs84_to_relative_utm(origin, this, x, y);
    }
    dx = x - wp_x;
    dy = y - wp_y;
}


std::ostream& operator<<(std::ostream& os, const Waypoint& wp) {

    os << wp.name.toStdString() << " : ";
    if(wp.type == Waypoint::WGS84) {
        os << wp.lat << "N, " << wp.lon << "E";
    } else {
        os << "not implemented!";//<< wp.x << "m, " << wp.y << "m";
    }

    os << ", " << wp.alt;

    if(wp.alt_type == Waypoint::ALT) {
        os << "m AMSL";
    } else {
        os << "m AGL";
    }

    return os;
}


