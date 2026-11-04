//
// Created by fabien on 22/02/2020.
//

#ifndef PPRZGCS_AIRCRAFTMANAGER_H
#define PPRZGCS_AIRCRAFTMANAGER_H

#include <QColor>
#include "aircraft.h"
#include <optional>
#include <functional>
#include <map>
#include <pprzlinkQt/Message.h>
#include <QMap>
#include <QSet>
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
    QMap<QString, QDomDocument> getAirframeIncludes() {return airframe_includes;}
    /// the uri each include was loaded from, by href
    QMap<QString, QString> getAirframeIncludeUris() {return airframe_include_uris;}
    QDomDocument getSettings() {return settings;}

    QString getFlightPlanUri() {return uri_flight_plan;}
    QString getAirframeUri() { return uri_airframe;}

    QString getId() {return ac_id;}
    QString getName() {return ac_name;}
    QColor getColor() {return color;}

    bool isReal() {return real;}
    void setReal(bool r) {real = r;}

    bool isComplete() {
        return !flight_plan.isNull() &&
               !airframe.isNull() &&
               pending_includes == 0 &&
               !settings.isNull();
    }

signals:
    void configReady(ConfigData*);

private:

    void setData(QDomDocument* doc, QString uri);
    void fetch(QString uri, std::function<void(QByteArray)> callback, std::function<void()> on_error = nullptr);
    void checkComplete();
    void loadAirframeIncludes();

    QString ac_id;
    QString ac_name;
    QColor color;

    QDomDocument flight_plan;
    QDomDocument airframe;
    QMap<QString, QDomDocument> airframe_includes;  // by href
    QMap<QString, QString> airframe_include_uris;   // by href
    int pending_includes = 0;
    bool ready_sent = false;
    // QDomDocument radio;
    QDomDocument settings;

    QString uri_flight_plan;
    QString uri_airframe;

    bool real;
};



class AircraftManager : public PprzTool{
    Q_OBJECT
public:
    explicit AircraftManager(PprzApplication* app, PprzToolbox* toolbox);
    virtual void setToolbox(PprzToolbox* toolbox) override;
    static AircraftManager* get() {
        return pprzApp()->toolbox()->aircraftManager();
    }

    Aircraft* getAircraft(QString id);
    optional<Aircraft*> getAircraftByName(QString name);
    QList<Aircraft*> getAircrafts();
    void newAircraftConfig(pprzlink::Message msg);
    bool aircraftExists(QString id);
    void removeAircraft(QString ac_id);
    /// Select this aircraft again when it is added (e.g. after a reload)
    void selectWhenAdded(QString ac_id) {select_when_added.insert(ac_id);}

    void addFPAircraft(QString ac_id, QString flightplan);

signals:
    void waypoint_changed(Waypoint*, QString ac_id);    //wp moved from backend
    void waypoint_added(Waypoint*, QString ac_id);      //new waypoint (for flightplan edition)

private:
    /// the color of an aircraft: the one chosen by the user (Aircrafts > Colors), else its
    /// configured one, or another one when an other aircraft already has (nearly) that color
    QColor aircraftColor(QString id, QString name, QColor configured);
    QMap<QString, QColor> assigned_colors;      // per aircraft id
    QMap<QString, QColor> configured_colors;    // the configured color it was assigned for
    static QColor parseColor(QString str);
    QDomDocument getXml(QString uri);

    QMap<QString, Aircraft*> aircrafts;
    QSet<QString> select_when_added;

private slots:
    void addAircraft(ConfigData* config);
    void moveWaypoint(pprzlink::Message msg);

};




#endif //PPRZGCS_AIRCRAFTMANAGER_H
