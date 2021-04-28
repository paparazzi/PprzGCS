//
// Created by fabien on 22/02/2020.
//

#include "AircraftManager.h"
#include <QDebug>
#include <QApplication>
#include "flightplan.h"
#include "setting_menu.h"
#include "airframe.h"
#include <QSettings>

AircraftManager::AircraftManager(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

Aircraft& AircraftManager::getAircraft(QString id) {
    if(aircrafts.find(id) != aircrafts.end()) {
        return aircrafts[id];
    } else {
        throw runtime_error("No such aircraft!");
    }
}

QList<Aircraft> AircraftManager::getAircrafts() {
    return aircrafts.values();
}

void AircraftManager::addAircraft(pprzlink::Message msg) {
    QSettings app_settings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);

    QString id, ac_name, default_gui_color, flight_plan, airframe, radio, settings;
    msg.getField("ac_id", id);
    msg.getField("ac_name", ac_name);
    msg.getField("default_gui_color", default_gui_color);
    msg.getField("flight_plan", flight_plan);
    msg.getField("airframe", airframe);
    msg.getField("radio", radio);
    msg.getField("settings", settings);

    QColor color = parseColor(default_gui_color);

    if(aircraftExists(id)) {
        qDebug() << "Aircraft " << id << " already exits!";
        return;
    }

    FlightPlan fp(flight_plan);
    shared_ptr<SettingMenu> sm = make_shared<SettingMenu>(settings);

    Airframe air(airframe);

    QString icon = app_settings.value("path/aircraft_icon").toString() + "/" + QString(air.getIconName()) + ".svg";

    aircrafts[id] = Aircraft(id, color, icon, ac_name, fp, sm, air);
}

bool AircraftManager::aircraftExists(QString id) {
    if(aircrafts.find(id) != aircrafts.end()) {
        return true;
    } else {
        return false;
    }
}

void AircraftManager::removeAircraft(QString ac_id) {
    (void)ac_id;
    if(aircraftExists(ac_id)) {
        aircrafts.remove(ac_id);
    }
}


QColor AircraftManager::parseColor(QString str) {
    QColor color = QColor();
    QSettings settings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);

    if(str[0] == '#' && str.size() == 13) {
        int r = str.mid(1, 4).toInt(nullptr, 16) >> 8;
        int g = str.mid(5, 4).toInt(nullptr, 16) >> 8;
        int b = str.mid(9, 4).toInt(nullptr, 16) >> 8;
        color = QColor(r, g, b);
    } else {
        color = QColor(str);
    }

    if(!color.isValid()) {
        color = QColor(settings.value("aircraft_default_color").toString());
    }

    return color;
}
