#include "flightplan.h"
#include "iostream"

#include <clocale>
#include "coordinatestransform.h"

using namespace std;
using namespace tinyxml2;

FlightPlan::FlightPlan(): origin()
{

}


FlightPlan::FlightPlan(string uri) : origin()
{
    setlocale(LC_ALL, "C"); // needed for stod() to use '.' as decimal separator instead of ',' (at least in France)
    XMLDocument doc;

    if(uri.substr(0,4) == "file") {
        string path = uri.substr(7, uri.length()-7);
        doc.LoadFile(path.c_str());
    }

    if(doc.Error()) {
        cerr << "Error parsing " << uri << ": " << doc.ErrorStr();
    }
    else {
        XMLElement* fp_root = doc.FirstChildElement( "dump" )->FirstChildElement( "flight_plan" );
        const char* lat0 = fp_root->Attribute("lat0");
        const char* lon0 = fp_root->Attribute("lon0");
        const char* defalt = fp_root->Attribute("alt");

        if(lat0 == nullptr || lon0 == nullptr || defalt == nullptr) {
            throw runtime_error("lat0, lon0 or alt not filled!");
        }

        defaultAlt = stod(defalt);

        origin = make_shared<Waypoint>("__ORIGIN", 0, stod(lat0), stod(lon0), defaultAlt);

        XMLElement* wps = fp_root->FirstChildElement("waypoints");
        parse_waypoints(wps);
        XMLElement* blks = fp_root->FirstChildElement("blocks");
        parse_blocks(blks);
    }
}

void FlightPlan::parse_waypoints(XMLElement* wps) {
    CoordinatesTransform ct;
    ct.init_WGS84_UTM(origin->getLat(), origin->getLon());

    XMLElement* wp = wps->FirstChildElement();
    uint8_t wp_id = 1;
    while(wp != nullptr) {
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

        if(lat_p != nullptr && lon_p != nullptr) {
            waypoints.push_back(make_shared<Waypoint>(name_p, wp_id, stod(lat_p), stod(lon_p), alt));
            if(altType == Waypoint::WpAltType::HEIGHT) {
                throw runtime_error("Unimplemented! Can't (yet) create absolute waypoint with relative height!");
            }
        }
        else if(x_p !=nullptr && y_p != nullptr) {
            double lat, lon;
            ct.relative_to_wgs84(origin->getLat(), origin->getLon(), stod(x_p), stod(y_p), &lat, &lon);
            waypoints.push_back(make_shared<Waypoint>(name_p, wp_id, lat, lon, alt, origin, altType));
        } else {
            throw runtime_error("You must specify either x/y or lat/lon!");
        }
        wp = wp->NextSiblingElement();
        ++wp_id;
    }
}

void FlightPlan::parse_blocks(tinyxml2::XMLElement* blks) {
    XMLElement* blk = blks->FirstChildElement();

    while(blk != nullptr) {
        auto block = make_shared<Block>(blk);
        blocks.push_back(block);
        blk = blk->NextSiblingElement();
    }

    std::sort(blocks.begin(), blocks.end(),
              [](auto lblock, auto rblock) {
                    return lblock->getNo() < rblock->getNo();
    });
}

shared_ptr<Waypoint> FlightPlan::getWaypoint(uint8_t id) {
    for(auto& wp: waypoints) {
        if(wp->getId() == id) {
            return wp;
        }
    }
    throw runtime_error("No waypoint with id " + to_string(id));
}

shared_ptr<Block> FlightPlan::getBlock(uint8_t no) {
    for(auto& bl: blocks) {
        if(bl->getNo() == no) {
            return bl;
        }
    }
    throw runtime_error("No block with no " + to_string(no));
}

vector<shared_ptr<BlockGroup>> FlightPlan::getGroups()
{
    if(blocks.size() == 0) {
        return vector<shared_ptr<BlockGroup>>();
    }

    vector<shared_ptr<BlockGroup>> v;

    shared_ptr<BlockGroup> bg = make_shared<BlockGroup>();
    bg->group_name = blocks[0]->getGroup();
    v.push_back(bg);

    for(auto b: blocks) {
        if(b->getGroup() != v[v.size()-1]->group_name) {
            shared_ptr<BlockGroup> bg = make_shared<BlockGroup>();
            bg->group_name = b->getGroup();
            v.push_back(bg);
        }
        v[v.size()-1]->blocks.push_back(b);
    }

    return v;
}
