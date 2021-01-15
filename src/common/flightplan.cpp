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
        name = fp_root->Attribute("name");
        const char* lat0 = fp_root->Attribute("lat0");
        const char* lon0 = fp_root->Attribute("lon0");
        const char* defalt = fp_root->Attribute("alt");
        const char* max_dist_home = fp_root->Attribute("max_dist_from_home");
        const char* gnd_alt = fp_root->Attribute("ground_alt");
        const char* sec_h = fp_root->Attribute("security_height");

        defaultAlt = stod(defalt);
        max_dist_from_home = stod(max_dist_home);
        ground_alt = stod(gnd_alt);
        security_height = stod(sec_h);

        origin = make_shared<Waypoint>("__ORIGIN", 0, stod(lat0), stod(lon0), defaultAlt);

        XMLElement* wps = fp_root->FirstChildElement("waypoints");
        parse_waypoints(wps);
        XMLElement* blks = fp_root->FirstChildElement("blocks");
        parse_blocks(blks);

        XMLElement* exs = fp_root->FirstChildElement("exceptions");
        if(exs) {
            parse_exceptions(exs);
        }

        XMLElement* vars = fp_root->FirstChildElement("variables");
        if(vars) {
            parse_variables(vars);
        }

        XMLElement* secs = fp_root->FirstChildElement("sectors");
        if(secs) {
            parse_sectors(secs);
        }

    }
}

void FlightPlan::parse_exceptions(XMLElement* exs) {
    for(auto ex=exs->FirstChildElement(); ex!=nullptr; ex=ex->NextSiblingElement()) {
        const char* cond = ex->Attribute("cond");
        const char* deroute = ex->Attribute("deroute");

        auto e = make_shared<Exception>();
        e->cond = cond;
        e->deroute = deroute;

        exceptions.push_back(e);
    }
}

void FlightPlan::parse_variables(XMLElement* vars) {
    for(auto ele=vars->FirstChildElement(); ele!=nullptr; ele=ele->NextSiblingElement()) {
        shared_ptr<Variable> var;
        if(strcmp(ele->Name(), "variable") == 0) {
            var = make_shared<Variable>(Variable::VARIABLE, ele->Attribute("var"));
        } else if (strcmp(ele->Name(), "abi_binding") == 0) {
            var = make_shared<Variable>(Variable::ABI_BINDING, ele->Attribute("name"));
        } else {
            throw std::runtime_error("");
        }
        for(auto att=ele->FirstAttribute(); att!=nullptr; att=att->Next()) {
            var->attributes[att->Name()] = att->Value();
        }
        variables.push_back(var);
    }
}

void FlightPlan::parse_sectors(tinyxml2::XMLElement* secs) {
    for(auto ele=secs->FirstChildElement(); ele!=nullptr; ele=ele->NextSiblingElement()) {
        shared_ptr<Sector> sec;
        if(strcmp(ele->Name(), "sector") == 0) {
            // It's a sector!
            sec = make_shared<Sector>(Sector::SECTOR);
            sec->name = ele->Attribute("name");
            auto color = ele->Attribute("color");
            if(color != nullptr) {
                sec->color = color;
            }
            auto type = ele->Attribute("type");
            if(type != nullptr) {
                sec->type = type;
            }
            for(auto corner=ele->FirstChildElement(); corner!= nullptr; corner=corner->NextSiblingElement()) {
                auto corner_name = corner->Attribute("name");
                sec->corners.push_back(corner_name);
            }
        } else if (strcmp(ele->Name(), "kml") == 0) {
            //It's a KML!
            sec = make_shared<Sector>(Sector::KML);
            sec->kml_file = ele->Attribute("file");
        } else {
            throw std::runtime_error("");
        }

        sectors.push_back(sec);
    }
}

void FlightPlan::parse_waypoints(XMLElement* wps) {
    CoordinatesTransform::get()->init_WGS84_UTM(origin->getLat(), origin->getLon());

    uint8_t wp_id = 1;
    for(auto wp=wps->FirstChildElement(); wp!=nullptr; wp=wp->NextSiblingElement()) {
        auto waypoint = make_shared<Waypoint>(wp, wp_id, origin, defaultAlt);
        waypoints.push_back(waypoint);
        ++wp_id;
    }
}

void FlightPlan::parse_blocks(tinyxml2::XMLElement* blks) {

    for(auto blk=blks->FirstChildElement(); blk!=nullptr; blk=blk->NextSiblingElement()) {
        auto block = make_shared<Block>(blk);
        blocks.push_back(block);
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

    std::map<std::string, shared_ptr<BlockGroup>> groups_map;

    for(auto b: blocks) {
        if(groups_map.find(b->getGroup()) == groups_map.end()) {
            groups_map[b->getGroup()] = make_shared<BlockGroup>();
            groups_map[b->getGroup()]->group_name = b->getGroup();
        }
        groups_map[b->getGroup()]->blocks.push_back(b);
    }


    vector<shared_ptr<BlockGroup>> groups;

    for( auto it = groups_map.begin(); it != groups_map.end(); ++it ) {
        groups.push_back( it->second );
    }

    sort(groups.begin(), groups.end(),
        [](shared_ptr<BlockGroup> lb, shared_ptr<BlockGroup> rb) {
                return lb->group_name < rb->group_name;
    });

    return groups;

}
