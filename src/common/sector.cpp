#include "sector.h"

Sector::Sector()
{

}

Sector::Sector(vector<shared_ptr<Waypoint>> corners, string name, optional<string> color):
    name(name), color(color), corners(corners)
{

}
