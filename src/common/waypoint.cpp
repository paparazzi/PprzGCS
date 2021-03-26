#include "waypoint.h"
#include <math.h>
#include <iostream>
#include "coordinatestransform.h"
#include "gcs_utils.h"
#include <QDebug>

using namespace tinyxml2;

Waypoint::Waypoint(string name, uint8_t id) :
    type(WGS84), id(id), lat(0), lon(0), origin(nullptr), alt(0), name(name)
{

}

Waypoint::Waypoint(string name, uint8_t id, double lat, double lon, double alt):
    Waypoint(name, id)
{
    type = WGS84;
    this->lat = lat;
    this->lon = lon;
    this->alt = alt;
    alt_type = ALT;
}



Waypoint::Waypoint(XMLElement* wp, uint8_t wp_id, shared_ptr<Waypoint> orig, double defaultAlt, WpFrame frame_type) {
    const char* name_p = wp->Attribute("name");
    const char* lat_p = wp->Attribute("lat");
    const char* lon_p = wp->Attribute("lon");
    const char* x_p = wp->Attribute("x");
    const char* y_p = wp->Attribute("y");
    const char* alt_p = wp->Attribute("alt");
    const char* height_p = wp->Attribute("height");

    Waypoint::WpAltType altType = Waypoint::WpAltType::ALT;
    double alt;
    if(height_p != nullptr) {
        altType = Waypoint::WpAltType::HEIGHT;
        alt = stod(height_p);
    } else if(alt_p != nullptr) {
        alt = stod(alt_p);
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
            throw runtime_error("Unimplemented! Can't (yet) create absolute waypoint with relative height!");
        }
    }
    else if(x_p !=nullptr && y_p != nullptr) {
        if(frame_type == WpFrame::UTM) {
            auto latlon = CoordinatesTransform::get()->relative_utm_to_wgs84(orig, stod(x_p), stod(y_p));
            this->lat = latlon.lat();
            this->lon = latlon.lon();
        } else if(frame_type == WpFrame::LTP) {
            auto latlon = CoordinatesTransform::get()->ltp_to_wgs84(orig, stod(x_p), stod(y_p));
            this->lat = latlon.lat();
            this->lon = latlon.lon();
        }

        type = frame_type;
        this->alt = alt;
        this->origin = orig;
        this->alt_type = altType;
    } else {
        throw runtime_error("You must specify either x/y or lat/lon!");
    }

    for(auto att=wp->FirstAttribute(); att != nullptr; att=att->Next()) {
        xml_attibutes[att->Name()] = att->Value();
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


ostream& operator<<(ostream& os, const Waypoint& wp) {

    os << wp.name << " : ";
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


