#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <QColor>
#include "flightplan.h"
#include "setting_menu.h"
#include "point2dlatlon.h"

class Aircraft
{
public:
    Aircraft();
    Aircraft(QString id, QColor color, QString icon, QString name, FlightPlan fp, shared_ptr<SettingMenu> setting_menu);

    QColor getColor(){return color;}
    QString getId(){return ac_id;}
    QString getIcon(){return icon;}
    QString name() {return _name;}
    FlightPlan& getFlightPlan() {return flight_plan;}
    //SettingMenu& getSettingMenu() {return setting_menu;}
    shared_ptr<SettingMenu> getSettingMenu() {return setting_menu;}

    Point2DLatLon getPosition() {return position;}
    void setPosition(Point2DLatLon pos) {position = pos;}

private:
    QString ac_id;
    QColor color;
    QString icon;
    QString _name;
    FlightPlan flight_plan;
    shared_ptr<SettingMenu> setting_menu;
    //SettingMenu setting_menu;

    Point2DLatLon position;

};

#endif // AIRCRAFT_H
