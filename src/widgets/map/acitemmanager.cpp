#include "acitemmanager.h"

ACItemManager::ACItemManager(QString ac_id, WaypointItem* target, AircraftItem* aircraft_item, ArrowItem* arrow, QObject* parent):
    QObject(parent),
    ac_id(ac_id), target(target), aircraft_item(aircraft_item),
    current_nav_shape(nullptr), max_dist_circle(nullptr), arrow_item(arrow)
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

void ACItemManager::setMaxDistCircle(CircleItem* ci) {
    max_dist_circle = ci;
}

void ACItemManager::removeItems(MapWidget* map) {
    for(auto &pi: pathItems) {
        map->removeItem(pi);
    }
    pathItems.clear();

    for(auto &wi: waypointItems) {
        map->removeItem(wi);
    }
    waypointItems.clear();

    if(target) {
        map->removeItem(target);
        target = nullptr;
    }

    if(current_nav_shape) {
        map->removeItem(current_nav_shape);
        current_nav_shape = nullptr;
    }

    if(aircraft_item) {
        map->removeItem(aircraft_item);
        aircraft_item = nullptr;
    }

    if(arrow_item) {
        map->removeItem(arrow_item);
        arrow_item = nullptr;
    }

    if(max_dist_circle) {
        map->removeItem(max_dist_circle);
        max_dist_circle = nullptr;
    }
}
