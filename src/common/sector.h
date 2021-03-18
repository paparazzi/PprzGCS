#ifndef SECTOR_H
#define SECTOR_H

#include <string>
#include <vector>
#include "waypoint.h"
#include <memory>

using namespace tinyxml2;
using namespace std;
class Sector
{
public:

    enum Type {
        STATIC,
        DYNAMIC,
    };

    Sector();
    Sector(vector<shared_ptr<Waypoint>> corners, string name, Type type=STATIC, optional<string> color=nullopt);

    string getName() {return name;}
    Type getType() {return type;}
    vector<shared_ptr<Waypoint>> getCorners() {return corners;}

private:
    string name;
    Type type;
    optional<string> color;   // override AC color
    vector<shared_ptr<Waypoint>> corners;
};

#endif // SECTOR_H
