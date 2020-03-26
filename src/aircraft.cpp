#include "aircraft.h"

Aircraft::Aircraft(QString id, QColor color, QString icon, QString n, FlightPlan fp, SettingMenu setting_menu):
    ac_id(id), color(color), icon(icon), _name(n), flight_plan(fp), setting_menu(setting_menu)
{

}
