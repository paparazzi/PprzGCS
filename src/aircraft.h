#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <QColor>
#include "flightplan.h"

class Aircraft
{
public:
    Aircraft(){}
    Aircraft(QString id, QColor color, QString icon, QString name, FlightPlan fp);

    QColor getColor(){return color;}
    QString getId(){return ac_id;}
    QString getIcon(){return icon;}
    QString name() {return _name;}
    FlightPlan& getFlightPlan() {return flight_plan;}

private:
    QString ac_id;
    QColor color;
    QString icon;
    QString _name;
    FlightPlan flight_plan;
};

#endif // AIRCRAFT_H
