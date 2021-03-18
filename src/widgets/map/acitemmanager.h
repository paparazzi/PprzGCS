#ifndef ACITEMMANAGER_H
#define ACITEMMANAGER_H

#include "map_item.h"
#include "waypoint_item.h"
#include "aircraft_item.h"
#include "path_item.h"

class ACItemManager
{
public:
    ACItemManager(QString ac_id, WaypointItem* target, AircraftItem* aircraft_item);

    void addWaypointItem(WaypointItem*);
    void addPathItem(PathItem*);
    void setCurrentNavShape(MapItem*);


    QList<WaypointItem*> getWaypointsItems() {return waypointItems;}
    WaypointItem* getTarget() {return target;}
    MapItem* getCurrentNavShape() {return current_nav_shape;}
    AircraftItem* getAircraftItem() {return aircraft_item;}

private:
    QString ac_id;
    QList<WaypointItem*> waypointItems;
    QList<PathItem*> pathItems;
    WaypointItem* target;
    AircraftItem* aircraft_item;
    MapItem* current_nav_shape;
};

#endif // ACITEMMANAGER_H
