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
#include <QtXml>
#include <QtNetwork>

class ConfigData: public QObject
{
    Q_OBJECT
public:

    ConfigData(QString ac_id, QString ac_name, QColor color, QObject* parent = nullptr);

    void setFlightPlan(QString uri);
    void setAirframe(QString uri);
    void setSettings(QString uri);

    QDomDocument getFlightPlan() {return flight_plan;}
    QDomDocument getAirframe() {return airframe;}
    QDomDocument getSettings() {return settings;}

    QString getId() {return ac_id;}
    QString getName() {return ac_name;}
    QColor getColor() {return color;}

    bool isComplete() {
        return !flight_plan.isNull() &&
               !airframe.isNull() &&
               !settings.isNull();
    }

signals:
    void configReady(ConfigData*);

private:

    void setData(QDomDocument* doc, QString uri);

    QString ac_id;
    QString ac_name;
    QColor color;

    QDomDocument flight_plan;
    QDomDocument airframe;
    // QDomDocument radio;
    QDomDocument settings;
};



class AircraftManager : public PprzTool{
    Q_OBJECT
public:
    explicit AircraftManager(PprzApplication* app, PprzToolbox* toolbox);
    static AircraftManager* get() {
        return pprzApp()->toolbox()->aircraftManager();
    }

    Aircraft& getAircraft(QString id);
    QList<Aircraft> getAircrafts();
    void newAircraftConfig(pprzlink::Message msg);
    bool aircraftExists(QString id);
    void removeAircraft(QString ac_id);

private:
    static QColor parseColor(QString str);
    QDomDocument getXml(QString uri);

    QMap<QString, Aircraft> aircrafts;

private slots:
    void addAircraft(ConfigData* config);

};




#endif //PPRZGCS_AIRCRAFTMANAGER_H
