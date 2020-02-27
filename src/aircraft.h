#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <QColor>

class Aircraft
{
public:
    Aircraft(){}
    Aircraft(QString id, QColor color, QString icon, QString name);

    QColor getColor(){return color;}
    QString getId(){return ac_id;}
    QString getIcon(){return icon;}
    QString name() {return _name;}

private:
    QString ac_id;
    QColor color;
    QString icon;
    QString _name;
};

#endif // AIRCRAFT_H
