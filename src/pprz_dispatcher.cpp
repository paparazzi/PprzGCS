#include "pprz_dispatcher.h"
#include <iostream>
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include <QDebug>
#include <QApplication>

using namespace std;

PprzDispatcher* PprzDispatcher::singleton = nullptr;

Q_DECLARE_METATYPE(pprzlink::Message);

PprzDispatcher::PprzDispatcher(QObject *parent) : QObject (parent), first_msg(false)
{
    std::string ivy_name = qApp->property("IVY_NAME").toString().toStdString();
    pprzlink_id = qApp->property("PPRZLINK_ID").toString().toStdString();

    dict = std::make_unique<pprzlink::MessageDictionary>(qApp->property("PPRZLINK_MESSAGES").toString().toStdString());
    link = std::make_unique<pprzlink::IvyLink>(*dict, ivy_name, qApp->property("IVY_BUS").toString().toStdString(), true);

    qRegisterMetaType<pprzlink::Message>();

    link->BindMessage(dict->getDefinition("FLIGHT_PARAM"),
        [=](std::string ac_id, pprzlink::Message msg) {
            (void)ac_id;
            std::string id;
            msg.getField("ac_id", id);

            if(AircraftManager::get()->aircraftExists(id.c_str())) {
                emit(flight_param(msg));
                if(!first_msg) {
                    first_msg = true;
                    emit(DispatcherUi::get()->ac_selected(QString(id.c_str())));
                }
            }
        }
    );


    link->BindMessage(dict->getDefinition("WAYPOINT_MOVED"),
        [=](std::string ac_id, pprzlink::Message msg) {
            (void)ac_id;
            std::string id;
            uint8_t wp_id = 200;
            float lat, lon, alt, ground_alt;
            msg.getField("ac_id", id);
            msg.getField("wp_id", wp_id);
            msg.getField("lat", lat);
            msg.getField("long", lon);
            msg.getField("alt", alt);
            msg.getField("ground_alt", ground_alt);
            if(AircraftManager::get()->aircraftExists(id.c_str()) && wp_id != 0) {
                Waypoint& wp = AircraftManager::get()->getAircraft(id.c_str()).getFlightPlan().getWaypoint(wp_id);
                wp.setLat(static_cast<double>(lat));
                wp.setLon(static_cast<double>(lon));
                wp.setAlt(static_cast<double>(alt));
                emit(waypoint_moved(id.c_str(), wp_id));
            }
    });



    connect(DispatcherUi::get(), &DispatcherUi::move_waypoint,
        [=](const Waypoint& wp, QString ac_id) {
            pprzlink::Message msg(dict->getDefinition("MOVE_WAYPOINT"));
            msg.setSenderId(pprzlink_id);
            msg.addField("ac_id", ac_id.toStdString());
            msg.addField("wp_id", wp.getId());
            msg.addField("lat", wp.getLat());
            msg.addField("long", wp.getLon());
            msg.addField("alt", wp.getAlt());
            link->sendMessage(msg);
        });



    usleep(10000);
    pprzlink::Message msg(dict->getDefinition("AIRCRAFTS_REQ"));
    msg.setSenderId(pprzlink_id);

    link->sendRequest(msg, [=](std::string ac_id, pprzlink::Message msg) {
        (void)ac_id;
        std::string ac_list;
        msg.getField("ac_list", ac_list);
        std::string id;
        std::stringstream ss(ac_list);
        while (std::getline(ss, id, ',')) {
            requestConfig(id);
        }

    });


    link->BindMessage(dict->getDefinition("NEW_AIRCRAFT"),
        [=](std::string ac_id, pprzlink::Message msg) {
            (void)ac_id;
            std::string id;
            msg.getField("ac_id", id);
            requestConfig(id);
        }
    );

}


void PprzDispatcher::requestConfig(std::string ac_id) {
    pprzlink::Message reqConfig(dict->getDefinition("CONFIG_REQ"));
    reqConfig.addField("ac_id", ac_id);
    reqConfig.setSenderId(pprzlink_id);
    link->sendRequest(reqConfig, [=](std::string ai, pprzlink::Message msg) {
        (void)ai;
        std::string ac_id;
        msg.getField("ac_id", ac_id);
        AircraftManager::get()->addAircraft(msg);
        emit(DispatcherUi::get()->new_ac_config(ac_id.c_str()));
    });
}

