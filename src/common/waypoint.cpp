#include "waypoint.h"
#include <math.h>
#include <iostream>

constexpr double EARTH_RADIUS = 6378137.0;

Waypoint::Waypoint(string name, uint8_t id) :
    type(ABSOLUTE), id(id), lat(0), lon(0), x(0), y(0), origin(nullptr), alt(0), name(name)
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

Waypoint::Waypoint(string name, uint8_t id, double x, double y, double alt, Waypoint* orig, WpAltType altType):
    Waypoint(name, id)
{
    type = RELATIVE;
    this->x = x;
    this->y = y;
    this->alt = alt;
    origin = std::make_shared<Waypoint>(*orig);
    this->alt_type = altType;
}


double Waypoint::getLon() const {
    if(type == WpType::ABSOLUTE) {
        return lon;
    } else {
        double longitude = origin->getLon();
        double latrad = origin->getLat() * M_PI/180.0;
        longitude += x/(EARTH_RADIUS*cos(latrad))*180.0/M_PI;
        return longitude;
    }
}

double Waypoint::getLat() const {
    if(type == WpType::ABSOLUTE) {
        return lat;
    } else {
        double latitude = origin->getLat();
        latitude += (y/EARTH_RADIUS)*180.0/M_PI;
        return latitude;
    }
}

void Waypoint::setLat(double lat) {
    if(type == WpType::ABSOLUTE) {
        this->lat = lat;
    } else {
        y += ((lat - getLat()) * M_PI/180.0) * EARTH_RADIUS;
    }
}

void Waypoint::setLon(double lon) {
    if(type == WpType::ABSOLUTE) {
        this->lon = lon;
    } else {
        double latrad = origin->getLat() * M_PI/180.0;
        x += ((lon - getLon()) * M_PI/180.0) * EARTH_RADIUS*cos(latrad);
    }
}


ostream& operator<<(ostream& os, const Waypoint& wp) {

    os << wp.name << " : ";
    if(wp.type == Waypoint::ABSOLUTE) {
        os << wp.lat << "N, " << wp.lon << "E";
    } else {
        os << wp.x << "m, " << wp.y << "m";
    }

    os << ", " << wp.alt;

    if(wp.alt_type == Waypoint::ALT) {
        os << "m AMSL";
    } else {
        os << "m AGL";
    }

    return os;
}


