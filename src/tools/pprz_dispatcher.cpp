#include "pprz_dispatcher.h"
#include <iostream>
#include "AircraftManager.h"
#include "dispatcher_ui.h"
#include <QDebug>
#include <PprzApplication.h>
#include <QDateTime>
#include <QTimer>
#include <QPointer>
#include "pprzmain.h"
#include <QSettings>
#include "gcs_utils.h"

using namespace std;

Q_DECLARE_METATYPE(pprzlink::Message)

// A message handler that throws would stop the GCS (and the control of the aircraft): log
// it and continue. Every message and request callback of the dispatcher goes through this.
static pprzlink::messageCallback_t guarded(QString name, pprzlink::messageCallback_t cb) {
    return [=](QString sender, pprzlink::Message msg) {
        try {
            cb(sender, msg);
        } catch(std::exception &e) {
            qWarning() << "Error handling" << name << "from" << sender << ":" << e.what();
        } catch(...) {
            qWarning() << "Error handling" << name << "from" << sender;
        }
    };
}

static long bindGuarded(pprzlink::IvyQtLink* link, const pprzlink::MessageDefinition& def, QObject* context, pprzlink::messageCallback_t cb) {
    return link->BindMessage(def, context, guarded(def.getName(), cb));
}

static long requestGuarded(pprzlink::IvyQtLink* link, const pprzlink::Message& msg, pprzlink::messageCallback_t cb) {
    return link->sendRequest(msg, guarded(msg.getDefinition().getName(), cb));
}

PprzDispatcher::PprzDispatcher(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox), started(false), silent_mode(false)
{

}

void PprzDispatcher::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);

    auto settings_path = appConfig()->value("SETTINGS_PATH").toString();

    QSettings settings(settings_path, QSettings::IniFormat);

    QString ivy_name = settings.value("ivy/name").toString();
    pprzlink_id = settings.value("pprzlink/id").toString();
    auto messages = appConfig()->value("MESSAGES").toString();

    dict = new pprzlink::MessageDictionary(messages);
    link = new pprzlink::IvyQtLink(*dict, ivy_name, this);

    connect(link, &pprzlink::IvyQtLink::serverConnected, this, [=,this]() {
        started = true;
       requestAircrafts();
    });

    qRegisterMetaType<pprzlink::Message>();

    connect(this, &PprzDispatcher::dl_values, this, &PprzDispatcher::updateSettings);

    connect(&server_check_timer, &QTimer::timeout, this, [this]() {
        auto now = QDateTime::currentMSecsSinceEpoch();
        bool ok = now - time_msg_server <= 1000;
        pprzApp()->mainWindow()->setServerStatus(ok);
        server_seen = server_seen || ok;
        // on a change, and once at the start without a server (after a few seconds)
        if(ok != server_ok || (!ok && !server_seen && ++server_checks == 3)) {
            server_ok = ok;
            emit serverStatusChanged(ok);
        }
    });


    long bid = bindGuarded(link, dict->getDefinition("WAYPOINT_MOVED"), nullptr,
        [=,this](QString ac_id, pprzlink::Message msg) {
            (void)ac_id;
            QString id;
            uint8_t wp_id = 0;
            msg.getField("ac_id", id);
            msg.getField("wp_id", wp_id);
            if(AircraftManager::get()->aircraftExists(id) && wp_id != 0) {
                emit(waypoint_moved(msg));
            }
    });
    _bindIds.append(bid);

    bid = bindGuarded(link, dict->getDefinition("AIRCRAFT_DIE"), nullptr,
        [=,this](QString ac_id, pprzlink::Message msg) {
            (void)ac_id;
            QString id;
            msg.getField("ac_id", id);

            if(AircraftManager::get()->aircraftExists(id)) {
                AircraftManager::get()->removeAircraft(id);
            }
        }
    );
    _bindIds.append(bid);

    // A rebooted aircraft is back at its initial state (flight plan, settings): reload it as
    // for AIRCRAFT_DIE + NEW_AIRCRAFT. This also gives it a new checklist for the new log.
    // (older Paparazzi versions don't have this message)
    bindOptional("AIRCRAFT_REBOOT", this,
        [=,this](QString sender, pprzlink::Message msg) {
            (void)sender;
            QString id;
            msg.getField("ac_id", id);
            // after the other handlers of this message (e.g. the chat notice)
            settings_after_reload.insert(id);
            QTimer::singleShot(0, this, [=,this]() {reloadAircraft(id);});
        }
    );

    bindDeftoSignal("AP_STATUS", &PprzDispatcher::ap_status);
    bindDeftoSignal("NAV_STATUS", &PprzDispatcher::nav_status);
    bindDeftoSignal("CIRCLE_STATUS", &PprzDispatcher::circle_status);
    bindDeftoSignal("SEGMENT_STATUS", &PprzDispatcher::segment_status);
    bindDeftoSignal("ENGINE_STATUS", &PprzDispatcher::engine_status);
    bindDeftoSignal("DL_VALUES", &PprzDispatcher::dl_values);
    bindDeftoSignal("TELEMETRY_STATUS", &PprzDispatcher::telemetry_status);
    bindDeftoSignal("FLY_BY_WIRE", &PprzDispatcher::fly_by_wire);
    bindDeftoSignal("FLIGHT_PARAM", &PprzDispatcher::flight_param);
    bindDeftoSignal("SVSINFO", &PprzDispatcher::svsinfo);


    connect(DispatcherUi::get(), &DispatcherUi::move_waypoint_ui, this,
        [=,this](Waypoint* wp, QString ac_id) {
            //Do not send the message if this is a "flight plan only" AC.
            if(AircraftManager::get()->getAircraft(ac_id)->isReal()) {
                pprzlink::Message msg(dict->getDefinition("MOVE_WAYPOINT"));
                msg.setSenderId(pprzlink_id);
                msg.addField("ac_id", ac_id);
                msg.addField("wp_id", wp->getId());
                msg.addField("lat", wp->getLat());
                msg.addField("long", wp->getLon());
                msg.addField("alt", wp->getAlt());
                if(started) {
                    this->sendMessage(msg);
                }
            }
        });

    // an aircraft loaded: the values of its settings asked. After a reboot all of them; else, once
    // the server had time to send those it knows, the ones still unknown (none for an aircraft
    // the server has seen for a while: no traffic for nothing)
    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, [=,this](QString id) {
        if(settings_after_reload.remove(id)) {
            requestSettings(id, false);
        } else {
            QTimer::singleShot(3000, this, [=,this]() {requestSettings(id, true);});
        }
    });

    bid = bindGuarded(link, dict->getDefinition("NEW_AIRCRAFT"), nullptr,
        [=,this](QString ac_id, pprzlink::Message msg) {
            (void)ac_id;
            QString id;
            msg.getField("ac_id", id);
            requestConfig(id);
        }
    );
    _bindIds.append(bid);
}

PprzDispatcher::~PprzDispatcher() {
    unbindAll();
}

void PprzDispatcher::unbindAll() {
    const QList<long> &constList = _bindIds;
    for(auto id: constList) {
        (void)id;
        link->UnbindMessage(id);
    }
    _bindIds.clear();
}

void PprzDispatcher::bindDeftoSignal(QString const &name, sig_ptr_t sig) {
    long bid = bindGuarded(link, dict->getDefinition(name), nullptr,
        [=,this](QString sender, pprzlink::Message msg) {
            if(sender == "ground") {
                time_msg_server = QDateTime::currentMSecsSinceEpoch();
            }

            QString id;
            msg.getField("ac_id", id);

            if(AircraftManager::get()->aircraftExists(id)) {
                if(DispatcherUi::get()->getSelectedAcId() == "") {
                    emit(DispatcherUi::get()->ac_selected(QString(id)));
                }
                emit((this->*sig)(msg));
                AircraftManager::get()->getAircraft(id)->getStatus()->updateMessage(msg);
            }
        }
    );
    _bindIds.append(bid);
}

void PprzDispatcher::requestConfig(QString ac_id) {
    pprzlink::Message reqConfig(dict->getDefinition("CONFIG_REQ"));
    reqConfig.addField("ac_id", ac_id);
    reqConfig.setSenderId(pprzlink_id);
    requestGuarded(link, reqConfig, [=,this](QString ai, pprzlink::Message msg) {
        (void)ai;
        AircraftManager::get()->newAircraftConfig(msg);
    });
}

void PprzDispatcher::sendMessage(pprzlink::Message msg) {
    assert(started);
    if(!silent_mode) {
        msg.setSenderId(pprzlink_id);
        link->sendMessage(msg);
    }
}

void PprzDispatcher::requestAircrafts() {
    auto def_ar = dict->getDefinition("AIRCRAFTS_REQ");
    pprzlink::Message msg(def_ar);
    msg.setSenderId(pprzlink_id);

    requestGuarded(link, msg, [=,this](QString sender, pprzlink::Message msg) {
        (void)sender;
        QString ac_list;
        msg.getField("ac_list", ac_list);
        for(QString id: ac_list.split(",")) {
            if(id == "") {continue;}
            if(!AircraftManager::get()->aircraftExists(id)) {
                requestConfig(id);
            } else {
                qDebug() << id << " already exists.";
            }
        }
    });
}

long PprzDispatcher::bind(QString msg_name, pprzlink::messageCallback_t cb) {
    long ret = bindGuarded(link, dict->getDefinition(msg_name), nullptr, cb);
    _bindIds.append(ret);
    return ret;
}

long PprzDispatcher::bind(QString msg_name, QObject* context, pprzlink::messageCallback_t cb) {
    try
    {
         long ret = bindGuarded(link, dict->getDefinition(msg_name), context, cb);
        _bindIds.append(ret);
        return ret;
    }
    catch(const pprzlink::no_such_message& e)
    {
        std::cerr << e.what() << '\n';
    }
    return -1;
}

bool PprzDispatcher::hasMessage(QString msg_name) {
    try {
        (void)dict->getDefinition(msg_name);
        return true;
    } catch(const pprzlink::no_such_message&) {
        return false;
    }
}

long PprzDispatcher::bindOptional(QString msg_name, QObject* context, pprzlink::messageCallback_t cb) {
    if(!hasMessage(msg_name)) {
        return -1;
    }
    return bind(msg_name, context, cb);
}

void PprzDispatcher::reloadAircraft(QString id) {
    if(!AircraftManager::get()->aircraftExists(id)) {
        return;
    }
    if(DispatcherUi::get()->getSelectedAcId() == id) {
        AircraftManager::get()->selectWhenAdded(id);
    }
    AircraftManager::get()->removeAircraft(id);
    requestConfig(id);
}

void PprzDispatcher::requestSettings(QString ac_id, bool only_unknown) {
    if(!AircraftManager::get()->aircraftExists(ac_id) || !hasMessage("GET_DL_SETTING")) {
        return;
    }
    // (an option: with a very slow datalink, they come one by one from the aircraft)
    if(!getAppSettings().value("settings/request_values", true).toBool()) {
        return;
    }
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    if(!ac->isReal() || ac->getSettingMenu() == nullptr) {
        return;
    }
    QList<Setting*> todo;
    QList<uint8_t> numbers;
    for(auto s: ac->getSettingMenu()->getAllSettings()) {
        if(!numbers.contains(s->getNo()) && (!only_unknown || !s->getValue().has_value())) {
            numbers.append(s->getNo());
            todo.append(s);
        }
    }
    if(todo.isEmpty()) {
        return;
    }
    // 10 per second (the uplink is shared with the commands); the answers come back as usual
    auto timer = new QTimer(this);
    timer->setInterval(100);
    auto next = std::make_shared<int>(0);
    QPointer<Aircraft> loaded = ac;
    connect(timer, &QTimer::timeout, this, [=,this]() {
        // stopped when done, or when the aircraft is gone (or reloaded again: a new request)
        if(*next >= numbers.size() || loaded.isNull() || !AircraftManager::get()->aircraftExists(ac_id) ||
           AircraftManager::get()->getAircraft(ac_id) != loaded) {
            timer->stop();
            timer->deleteLater();
            return;
        }
        // (known meanwhile: not asked)
        while(only_unknown && *next < todo.size() && todo[*next]->getValue().has_value()) {
            *next += 1;
        }
        if(*next >= todo.size()) {
            return;     // stopped at the next tick
        }
        pprzlink::Message msg(dict->getDefinition("GET_DL_SETTING"));
        msg.setSenderId(pprzlink_id);
        msg.addField("ac_id", ac_id);
        msg.addField("index", numbers[*next]);
        if(started) {
            sendMessage(msg);
        }
        *next += 1;
    });
    timer->start();
}

void PprzDispatcher::unBind(long bid) {
    link->UnbindMessage(bid);
    _bindIds.removeAll(bid);
}

void PprzDispatcher::stop() {
    link->stop();
}

void PprzDispatcher::start() {
    QSettings settings(appConfig()->value("SETTINGS_PATH").toString(), QSettings::IniFormat);
    link->start(appConfig()->value("IVY_BUS").toString());

    server_check_timer.setInterval(1000);
    server_check_timer.start();
}


void PprzDispatcher::updateSettings(pprzlink::Message msg) {
    QString ac_id;
    QString values;
    msg.getField("ac_id", ac_id);
    msg.getField("values", values);
    QString id = QString(ac_id);

    auto ac = AircraftManager::get()->getAircraft(id);

    auto settings = ac->getSettingMenu()->getAllSettings();
    sort(settings.begin(), settings.end(),
        [](Setting* sl, Setting* sr) {
                return sl->getNo() < sr->getNo();
    });

    if(settings.size() == 0) {
        return;
    }

    size_t i=0;
    for(QString &token: values.split(",")) {
        if(settings.size() <= static_cast<int>(i)) {
            break;
        }
        if(token != "" && token != "?") {
            double s = token.toDouble();
            assert(settings[i]->getNo() == static_cast<uint8_t>(i));
            float value = static_cast<float>(s);
            settings[i]->setValue(value);
            emit DispatcherUi::get()->settingUpdated(ac_id, settings[i], value);
        }
        i++;
    }
}
