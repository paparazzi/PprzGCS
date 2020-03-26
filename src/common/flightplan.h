#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include <string>
#include <vector>
#include "waypoint.h"
#include "block.h"
#include <memory>
#include "tinyxml2.h"

struct BlockGroup {
    string group_name;
    vector<shared_ptr<Block>> blocks;
};

class FlightPlan
{
public:



    FlightPlan();
    FlightPlan(std::string uri);

    vector<shared_ptr<Waypoint>>& getWaypoints() {return  waypoints;}
    shared_ptr<Waypoint> getWaypoint(uint8_t id);
    vector<shared_ptr<Block>>& getBlocks() {return  blocks;}
    vector<shared_ptr<BlockGroup>> getGroups();
    shared_ptr<Block> getBlock(uint8_t id);
    double getDefaultAltitude() {return defaultAlt;}
    shared_ptr<Waypoint> getOrigin() {return origin;}

private:
    void parse_waypoints(tinyxml2::XMLElement* wps);
    void parse_blocks(tinyxml2::XMLElement* blks);
    void parse_block_stages(tinyxml2::XMLElement* blk, shared_ptr<Block> block);

    std::vector<shared_ptr<Waypoint>> waypoints;
    std::vector<shared_ptr<Block>> blocks;
    shared_ptr<Waypoint> origin;

    double defaultAlt;
};

#endif // FLIGHTPLAN_H
