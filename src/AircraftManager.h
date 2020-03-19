//
// Created by fabien on 22/02/2020.
//

#ifndef PPRZGCS_AIRCRAFTMANAGER_H
#define PPRZGCS_AIRCRAFTMANAGER_H

#include <QColor>
#include "aircraft.h"
#include <optional>
#include <map>
#include <pprzlink/Message.h>

class AircraftManager {
public:
    static AircraftManager* get() {
        if(singleton == nullptr) {
            singleton = new AircraftManager();
        }
        return singleton;
    }

    Aircraft& getAircraft(QString id);
    void addAircraft(pprzlink::Message msg);
    bool aircraftExists(QString id);

private:
    static AircraftManager* singleton;
    explicit AircraftManager();
    static QColor parseColor(std::string str);

    std::map<QString, Aircraft> aircrafts;

};


#endif //PPRZGCS_AIRCRAFTMANAGER_H
