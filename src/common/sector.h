#ifndef SECTOR_H
#define SECTOR_H

#include <string>
#include "waypoint.h"
#include <memory>

using namespace std;
class Sector
{
public:
    Sector();
    Sector(QList<Waypoint*> corners, QString name, optional<QString> color=nullopt);

    QString getName() {return name;}
    QList<Waypoint*> getCorners() {return corners;}

private:
    QString name;
    optional<QString> color;   // override AC color
    QList<Waypoint*> corners;
};

#endif // SECTOR_H
