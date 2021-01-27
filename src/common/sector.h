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
    Sector();
    Sector(vector<shared_ptr<Waypoint>> corners, string name, optional<string> color=nullopt);

    string getName() {return name;}
    vector<shared_ptr<Waypoint>> getCorners() {return corners;}

private:
    string name;
    optional<string> color;   // override AC color
    vector<shared_ptr<Waypoint>> corners;
};

#endif // SECTOR_H
