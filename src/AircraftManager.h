//
// Created by fabien on 22/02/2020.
//

#ifndef PPRZGCS_AIRCRAFTMANAGER_H
#define PPRZGCS_AIRCRAFTMANAGER_H

#include <QColor>
#include "aircraft.h"
#include <optional>
#include <map>
#include <pprzlinkQt/Message.h>
#include <QMap>
#include "PprzToolbox.h"
#include "PprzApplication.h"


class AircraftManager : public PprzTool{
public:
    explicit AircraftManager(PprzApplication* app, PprzToolbox* toolbox);
    static AircraftManager* get() {
        return pprzApp()->toolbox()->aircraftManager();
    }

    Aircraft& getAircraft(QString id);
    QList<Aircraft> getAircrafts();
    void addAircraft(pprzlink::Message msg);
    bool aircraftExists(QString id);
    void removeAircraft(QString ac_id);

private:
    static QColor parseColor(std::string str);

    QMap<QString, Aircraft> aircrafts;

};


#endif //PPRZGCS_AIRCRAFTMANAGER_H
