#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include <string>
#include <vector>
#include "waypoint.h"

class FlightPlan
{
public:
    FlightPlan();
    FlightPlan(std::string uri);

    std::vector<Waypoint>& getWaypoints() {return  waypoints;}
    Waypoint& getWaypoint(uint8_t id);
    Waypoint& getWaypoint(string id);
    double getDefaultAltitude() {return defaultAlt;}

private:
    std::vector<Waypoint> waypoints;
    Waypoint origin;

    double defaultAlt;
};

#endif // FLIGHTPLAN_H
