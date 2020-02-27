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

            if(!first_msg) {
                first_msg = true;
                emit(DispatcherUi::get()->ac_selected(QString(id.c_str())));
            }

            if(AircraftManager::get()->aircraftExists(id.c_str())) {
                emit(flight_param(msg));
            }
        }
    );

    usleep(100000);
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
    link->sendRequest(reqConfig, [=](std::string ac_id, pprzlink::Message msg) {
        (void)ac_id;
        AircraftManager::get()->addAircraft(msg);
    });
}

