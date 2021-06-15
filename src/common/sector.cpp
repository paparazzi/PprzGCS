#include "sector.h"

Sector::Sector()
{

}

Sector::Sector(QList<Waypoint*> corners, QString name, optional<QString> color):
    name(name), color(color), corners(corners)
{

}
