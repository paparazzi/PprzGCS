#include "sector.h"

Sector::Sector()
{

}

Sector::Sector(vector<shared_ptr<Waypoint>> corners, QString name, Type type, optional<QString> color):
    name(name), type(type), color(color), corners(corners)
{

}
