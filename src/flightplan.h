#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include <string>
#include <vector>
#include "waypoint.h"
#include <memory>

class FlightPlan
{
public:
    FlightPlan();
    FlightPlan(std::string uri);

    std::vector<shared_ptr<Waypoint>>& getWaypoints() {return  waypoints;}
    shared_ptr<Waypoint> getWaypoint(uint8_t id);
    double getDefaultAltitude() {return defaultAlt;}
    shared_ptr<Waypoint> getOrigin() {return origin;}

private:
    std::vector<shared_ptr<Waypoint>> waypoints;
    shared_ptr<Waypoint> origin;

    double defaultAlt;
};

#endif // FLIGHTPLAN_H
