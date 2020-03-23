#include "waypoint.h"
#include <math.h>
#include <iostream>


Waypoint::Waypoint(string name, uint8_t id) :
    type(ABSOLUTE), id(id), lat(0), lon(0), origin(nullptr), alt(0), name(name)
{

}

Waypoint::Waypoint(string name, uint8_t id, double lat, double lon, double alt):
    Waypoint(name, id)
{
    type = ABSOLUTE;
    this->lat = lat;
    this->lon = lon;
    this->alt = alt;
    alt_type = ALT;
}

Waypoint::Waypoint(string name, uint8_t id, double lat, double lon, double alt, shared_ptr<Waypoint> orig, WpAltType altType):
    Waypoint(name, id)
{
    type = RELATIVE;
    this->alt = alt;
    this->lat = lat;
    this->lon = lon;
    origin = orig;
    this->alt_type = altType;
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
    if(wp.type == Waypoint::ABSOLUTE) {
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


