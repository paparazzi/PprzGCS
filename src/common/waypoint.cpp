#include "waypoint.h"
#include <math.h>
#include <iostream>
#include "coordinatestransform.h"
#include "gcs_utils.h"
#include <QDebug>

Waypoint::Waypoint(Waypoint* original, QObject* parent):
    QObject(parent)
{
    type = original->type;
    id = original->id;
    lat = original->lat;
    lon = original->lat;
    origin = original;
    alt = original->alt;
    alt_type = original->alt_type;
    name = original->name;
    xml_attibutes = original->xml_attibutes;
}

Waypoint::Waypoint(QString name, uint8_t id, QObject* parent) :
    QObject(parent),
    type(WGS84), id(id), lat(0), lon(0), origin(nullptr), alt(0), name(name)
{

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
    QObject(parent)
{
    auto name_p = wp.attribute("name");
    auto lat_p = wp.attribute("lat");
    auto lon_p = wp.attribute("lon");
    auto x_p = wp.attribute("x");
    auto y_p = wp.attribute("y");

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

    name = name_p;
    id = wp_id;

    if(lat_p != nullptr && lon_p != nullptr) {

        type = WGS84;
        this->lat = parse_coordinate(lat_p);
        this->lon = parse_coordinate(lon_p);
        this->alt = alt;
        alt_type = ALT;

        if(altType == Waypoint::WpAltType::HEIGHT) {
            throw std::runtime_error("Unimplemented! Can't (yet) create absolute waypoint with relative height!");
        }
    }
    else if(x_p !=nullptr && y_p != nullptr) {
        if(frame_type == WpFrame::UTM) {
            auto latlon = CoordinatesTransform::get()->relative_utm_to_wgs84(orig, x_p.toDouble(), y_p.toDouble());
            this->lat = latlon.lat();
            this->lon = latlon.lon();
        } else if(frame_type == WpFrame::LTP) {
            auto latlon = CoordinatesTransform::get()->ltp_to_wgs84(orig, x_p.toDouble(), y_p.toDouble());
            this->lat = latlon.lat();
            this->lon = latlon.lon();
        }

        type = frame_type;
        this->alt = alt;
        this->origin = orig;
        this->alt_type = altType;
    } else {
        throw std::runtime_error("You must specify either x/y or lat/lon!");
    }

    auto attr = wp.attributes();
    for(int i=0; i < attr.count(); i++) {
        auto att = attr.item(i).toAttr();
       xml_attibutes[att.name()] = att.value();
    }

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


