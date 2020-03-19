#include "aircraft.h"

Aircraft::Aircraft(QString id, QColor color, QString icon, QString n, FlightPlan fp):
    ac_id(id), color(color), icon(icon), _name(n), flight_plan(fp)
{

}
