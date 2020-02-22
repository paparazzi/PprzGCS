//
// Created by fabien on 22/02/2020.
//

#ifndef PPRZGCS_AIRCRAFTMANAGER_H
#define PPRZGCS_AIRCRAFTMANAGER_H

#include <QColor>
#include <QMap>

class AircraftManager {
public:
    static AircraftManager* get() {
        if(singleton == nullptr) {
            singleton = new AircraftManager();
        }
        return singleton;
    }

    std::optional<QColor> getColor(int id);
    void addAircraft(int id, QColor color);

private:
    static AircraftManager* singleton;
    explicit AircraftManager();

    std::map<int, QColor> colors;

};


#endif //PPRZGCS_AIRCRAFTMANAGER_H
