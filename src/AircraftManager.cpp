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
#include "dispatcher_ui.h"

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


void AircraftManager::newAircraftConfig(pprzlink::Message msg) {
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

   auto config = new ConfigData(id, ac_name, color);

   connect(config, &ConfigData::configReady, this, &AircraftManager::addAircraft);

   config->setFlightPlan(flight_plan);
   config->setAirframe(airframe);
   config->setSettings(settings);
}

void AircraftManager::addAircraft(ConfigData* config) {
    QSettings app_settings(qApp->property("SETTINGS_PATH").toString(), QSettings::IniFormat);

    FlightPlan fp(config->getFlightPlan());
    shared_ptr<SettingMenu> sm = make_shared<SettingMenu>(config->getSettings());

    Airframe air(config->getAirframe());

    QString icon = app_settings.value("path/aircraft_icon").toString() + "/" + QString(air.getIconName()) + ".svg";

    aircrafts[config->getId()] = Aircraft(config->getId(), config->getColor(), icon, config->getName(), fp, sm, air);

    emit DispatcherUi::get()->new_ac_config(config->getId());

    config->deleteLater();
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
        int r = str.midRef(1, 4).toInt(nullptr, 16) >> 8;
        int g = str.midRef(5, 4).toInt(nullptr, 16) >> 8;
        int b = str.midRef(9, 4).toInt(nullptr, 16) >> 8;
        color = QColor(r, g, b);
    } else {
        color = QColor(str);
    }

    if(!color.isValid()) {
        color = QColor(settings.value("aircraft_default_color").toString());
    }

    return color;
}


ConfigData::ConfigData(QString ac_id, QString ac_name, QColor color, QObject* parent) :
    QObject(parent),
    ac_id(ac_id), ac_name(ac_name), color(color)
{
}

void ConfigData::setFlightPlan(QString uri) {
    flight_plan = getXml(uri);
    if(isComplete()) {
        emit configReady(this);
    }
}

void ConfigData::setAirframe(QString uri) {
    airframe = getXml(uri);
    if(isComplete()) {
        emit configReady(this);
    }
}

void ConfigData::setSettings(QString uri) {
    settings = getXml(uri);
    if(isComplete()) {
        emit configReady(this);
    }
}


QDomDocument ConfigData::getXml(QString uri) {
    QDomDocument doc;

    if(uri.mid(0,4) == "file") {
        QString path = uri.mid(7, uri.length()-7);
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Error while loading flightplan file");
        }
        doc.setContent(&f);
        f.close();
    } else {
        throw std::runtime_error("Unimplemented ! " + uri.toStdString());
    }

    return doc;
}
