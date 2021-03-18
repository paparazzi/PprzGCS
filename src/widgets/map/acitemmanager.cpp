#include "acitemmanager.h"

ACItemManager::ACItemManager(QString ac_id, WaypointItem* target, AircraftItem* aircraft_item):
    ac_id(ac_id), target(target), aircraft_item(aircraft_item), current_nav_shape(nullptr)
{

}

void ACItemManager::addWaypointItem(WaypointItem* wi) {
    waypointItems.append(wi);
}

void ACItemManager::addPathItem(PathItem* pi) {
    pathItems.append(pi);
}

void ACItemManager::setCurrentNavShape(MapItem* mi) {
    current_nav_shape = mi;
}
