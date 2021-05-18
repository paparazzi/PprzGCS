#ifndef SECTOR_H
#define SECTOR_H

#include <string>
#include <vector>
#include "waypoint.h"
#include <memory>

using namespace std;
class Sector
{
public:

    enum Type {
        STATIC,
        DYNAMIC,
    };

    Sector();
    Sector(QList<Waypoint*> corners, QString name, Type type=STATIC, optional<QString> color=nullopt);

    QString getName() {return name;}
    Type getType() {return type;}
    QList<Waypoint*> getCorners() {return corners;}

private:
    QString name;
    Type type;
    optional<QString> color;   // override AC color
    QList<Waypoint*> corners;
};

#endif // SECTOR_H
