#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include <string>
#include <vector>
#include "waypoint.h"
#include "sector.h"
#include "block.h"
#include <memory>
#include "tinyxml2.h"

struct BlockGroup {
    string group_name;
    vector<shared_ptr<Block>> blocks;
};

struct Exception {
    string cond;
    string deroute;
};

struct Variable {
    enum Type {
        VARIABLE,
        ABI_BINDING,
    };

    Variable(Type t, string var_name) {
        _var_name = var_name;
        type = t;
    }

    Type type;
    string _var_name;
    map<string, string> attributes;
};


class FlightPlan
{
public:



    FlightPlan();
    FlightPlan(std::string uri);

    vector<shared_ptr<Waypoint>>& getWaypoints() {return  waypoints;}
    shared_ptr<Waypoint> getWaypoint(uint8_t id);
    shared_ptr<Waypoint> getWaypoint(string name);
    vector<shared_ptr<Block>>& getBlocks() {return  blocks;}
    vector<shared_ptr<BlockGroup>> getGroups();
    vector<shared_ptr<Exception>> getExeptions() {return  exceptions;}
    vector<shared_ptr<Variable>> getVariables() {return  variables;}
    vector<shared_ptr<Sector>> getSectors() {return  sectors;}
    shared_ptr<Block> getBlock(uint8_t id);
    double getDefaultAltitude() {return defaultAlt;}
    double getGroundAlt() {return ground_alt;}
    void setGroundAlt(double ga) {ground_alt = ga;}
    shared_ptr<Waypoint> getOrigin() {return origin;}

private:
    void parse_exceptions(tinyxml2::XMLElement* exs);
    void parse_variables(tinyxml2::XMLElement* vars);
    void parse_sectors(tinyxml2::XMLElement* secs);
    void parse_waypoints(tinyxml2::XMLElement* wps, Waypoint::WpFrame frame_type);
    void parse_blocks(tinyxml2::XMLElement* blks);
    void parse_block_stages(tinyxml2::XMLElement* blk, shared_ptr<Block> block);

    std::vector<shared_ptr<Waypoint>> waypoints;
    std::vector<shared_ptr<Exception>> exceptions;
    std::vector<shared_ptr<Block>> blocks;
    std::vector<shared_ptr<Variable>> variables;
    std::vector<shared_ptr<Sector>> sectors;
    shared_ptr<Waypoint> origin;

    double defaultAlt;
    string name;
    double max_dist_from_home;
    double ground_alt;
    double security_height;


};

#endif // FLIGHTPLAN_H
