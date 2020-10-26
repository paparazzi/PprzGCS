#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <memory>
#include <iostream>
#include <map>
#include "tinyxml2.h"

using namespace std;

class Waypoint
{
public:

    enum WpType {
        ABSOLUTE,
        RELATIVE
    };

    enum WpAltType {
        ALT,
        HEIGHT
    };

    Waypoint(string name, uint8_t id);
    Waypoint(string name, uint8_t id, double lat, double lon, double alt);
    Waypoint(string name, uint8_t id, double x, double y, double alt, shared_ptr<Waypoint> orig, WpAltType altType);
    Waypoint(tinyxml2::XMLElement* wp, uint8_t wp_id, shared_ptr<Waypoint> orig, double defaultAlt);

    uint8_t getId() const {return id;}
    double getLat() const;
    double getLon() const;
    void setLat (double lat);
    void setLon(double lon);
    void setAlt(double alt) {this->alt = alt;}
    double getAlt() const {return alt;}
    WpType getType() const {return type;}
    string getName() const {return name;}
    shared_ptr<Waypoint> getOrigin() {return origin;}
    std::map<string, string> getXmlAttributes() { return xml_attibutes;}

    friend ostream& operator<<(ostream& os, const Waypoint& wp);

private:
    WpType type;

    uint8_t id;

    double lat;
    double lon;

    shared_ptr<Waypoint> origin;

    double alt;
    WpAltType alt_type;

    string name;

    std::map<string, string> xml_attibutes;
};


#endif // WAYPOINT_H
