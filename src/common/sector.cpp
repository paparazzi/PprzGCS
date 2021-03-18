#include "sector.h"

Sector::Sector()
{

}

Sector::Sector(vector<shared_ptr<Waypoint>> corners, string name, Type type, optional<string> color):
    name(name), type(type), color(color), corners(corners)
{

}
