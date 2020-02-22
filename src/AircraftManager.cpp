//
// Created by fabien on 22/02/2020.
//

#include "AircraftManager.h"

AircraftManager* AircraftManager::singleton = nullptr;

AircraftManager::AircraftManager() {

}

std::optional<QColor> AircraftManager::getColor(int id) {
    if(colors.find(id) != colors.end()) {
        return colors[id];
    } else {
        return {};
    }
}

void AircraftManager::addAircraft(int id, QColor color) {
    colors[id] = color;
}
