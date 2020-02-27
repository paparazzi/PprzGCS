//
// Created by fabien on 22/02/2020.
//

#include "AircraftManager.h"
#include <QDebug>
#include <QApplication>

AircraftManager* AircraftManager::singleton = nullptr;

AircraftManager::AircraftManager() {

}

std::optional<Aircraft> AircraftManager::getAircraft(QString id) {
    if(aircrafts.find(id) != aircrafts.end()) {
        return aircrafts[id];
    } else {
        return {};
    }
}

void AircraftManager::addAircraft(QString id) {
    aircrafts[id] = Aircraft(id, Qt::red, "/home/fabien/DEV/test_qt/PprzGCS/resources/pictures/fixed_wing.svg", "plop");
    qDebug() << "aircraft " << id << " added!";
}

void AircraftManager::addAircraft(pprzlink::Message msg) {
    std::string ac_id;
    //std::string flight_plan;
    //std::string airframe;
    //...
    std::string ac_name;
    std::string default_gui_color;
    msg.getField("ac_id", ac_id);
    msg.getField("ac_name", ac_name);
    msg.getField("default_gui_color", default_gui_color);

    QColor color = parseColor(default_gui_color);

    QString id = QString::fromStdString(ac_id);

    if(aircraftExists(id)) {
        qDebug() << "Aircraft " << id << " already exits!";
        return;
    }

    aircrafts[id] = Aircraft(id, color, qApp->property("DEFAULT_AIRCRAFT_ICON").toString(), QString::fromStdString(ac_name));
}

bool AircraftManager::aircraftExists(QString id) {
    if(aircrafts.find(id) != aircrafts.end()) {
        return true;
    } else {
        return false;
    }
}


QColor AircraftManager::parseColor(std::string str) {
    QColor color = QColor();

    if(str[0] == '#' && str.size() == 13) {
        int r = std::stoi(str.substr(1, 4), nullptr, 16) >> 8;
        int g = std::stoi(str.substr(5, 4), nullptr, 16)  >> 8;
        int b = std::stoi(str.substr(9, 4), nullptr, 16)  >> 8;
        color = QColor(r, g, b);
    } else {
        color = QColor(str.c_str());
    }

    if(!color.isValid()) {
        color = QColor(qApp->property("DEFAULT_COLOR").toString());
    }

    return color;
}
