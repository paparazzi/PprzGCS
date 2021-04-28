#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include <QString>
#include <vector>
#include "waypoint.h"
#include "sector.h"
#include "block.h"
#include <memory>
#include <QtXml>
#include "point2dlatlon.h"

struct BlockGroup {
    QString group_name;
    vector<shared_ptr<Block>> blocks;
};

struct Exception {
    QString cond;
    QString deroute;
};

struct Variable {
    enum Type {
        VARIABLE,
        ABI_BINDING,
    };

    Variable(Type t, QString var_name) {
        _var_name = var_name;
        type = t;
    }

    Type type;
    QString _var_name;
    map<QString, QString> attributes;
};


class FlightPlan
{
public:

    FlightPlan();
    FlightPlan(QString uri);

    vector<shared_ptr<Waypoint>>& getWaypoints() {return  waypoints;}
    shared_ptr<Waypoint> getWaypoint(uint8_t id);
    shared_ptr<Waypoint> getWaypoint(QString name);
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
    Waypoint::WpFrame getFrame() {return frame_type;}

    /**
     * @brief boundingBox of the flightPlan (NW, SE)
     * @return NW, SE
     */
    std::tuple<Point2DLatLon, Point2DLatLon> boundingBox();

    /**
     * @brief boundingBoxWith boundingBox of the flightPlan + given point
     * @param pt
     * @return NW, SE
     */
    std::tuple<Point2DLatLon, Point2DLatLon> boundingBoxWith(Point2DLatLon pt);

private:
    void parse_exceptions(QDomElement exs);
    void parse_variables(QDomElement vars);
    void parse_sectors(QDomElement secs);
    void parse_waypoints(QDomElement wps);
    void parse_blocks(QDomElement blks);
    void parse_block_stages(QDomElement blk, shared_ptr<Block> block);

    std::vector<shared_ptr<Waypoint>> waypoints;
    std::vector<shared_ptr<Exception>> exceptions;
    std::vector<shared_ptr<Block>> blocks;
    std::vector<shared_ptr<Variable>> variables;
    std::vector<shared_ptr<Sector>> sectors;
    shared_ptr<Waypoint> origin;

    double defaultAlt;
    QString name;
    double max_dist_from_home;
    double ground_alt;
    double security_height;

    Waypoint::WpFrame frame_type;


};

#endif // FLIGHTPLAN_H
