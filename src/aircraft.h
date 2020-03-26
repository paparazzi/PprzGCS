#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <QColor>
#include "flightplan.h"
#include "setting_menu.h"

class Aircraft
{
public:
    Aircraft(){}
    Aircraft(QString id, QColor color, QString icon, QString name, FlightPlan fp, SettingMenu setting_menu);

    QColor getColor(){return color;}
    QString getId(){return ac_id;}
    QString getIcon(){return icon;}
    QString name() {return _name;}
    FlightPlan& getFlightPlan() {return flight_plan;}
    SettingMenu& getSettingMenu() {return setting_menu;}

private:
    QString ac_id;
    QColor color;
    QString icon;
    QString _name;
    FlightPlan flight_plan;
    SettingMenu setting_menu;
};

#endif // AIRCRAFT_H
