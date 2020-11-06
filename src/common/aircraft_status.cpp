#include "aircraft_status.h"
#include <QDebug>

namespace { std::mutex mtx; }

//#define DEBUG_THREADS

#ifdef DEBUG_THREADS
static std::set<std::thread::id> thread_ids;
#endif

AircraftStatus::AircraftStatus(QString ac_id, QObject *parent) : QObject(parent),
    ac_id(ac_id)
{

}

void AircraftStatus::updateMessage(pprzlink::Message msg) {
    const std::lock_guard<std::mutex> lock(mtx);

#ifdef DEBUG_THREADS
    auto thread_id = std::this_thread::get_id();
    thread_ids.insert(thread_id);
#endif

    std::string id;
    msg.getField("ac_id", id);
    if(id.c_str() == ac_id) {
        auto name = msg.getDefinition().getName();
        last_messages[name] = msg;

        if(name == "FLIGHT_PARAM") {
            emit(flight_param());
        }
        else if(name == "AP_STATUS") {
            emit(ap_status());
        }
        else if(name == "NAV_STATUS") {
            emit(nav_status());
        }
        else if(name == "CIRCLE_STATUS") {
            emit(circle_status());
        }
        else if(name == "SEGMENT_STATUS") {
            emit(segment_status());
        }
        else if(name == "ENGINE_STATUS") {
            emit(engine_status());
        }
        else if(name == "WAYPOINT_MOVED") {
            emit(waypoint_moved());
        }
        else if(name == "DL_VALUES") {
            emit(dl_values());
        }
        else if(name == "TELEMETRY_STATUS") {
            emit(telemetry_status());
        }
        else if(name == "FLY_BY_WIRE") {
            emit(fly_by_wire());
        }
        else if(name == "SVSINFO") {
            emit(svsinfo());
        }
    }
}

std::optional<pprzlink::Message> AircraftStatus::getMessage(std::string name) {
    const std::lock_guard<std::mutex> lock(mtx);

#ifdef DEBUG_THREADS
    auto thread_id = std::this_thread::get_id();
    thread_ids.insert(thread_id);
    qDebug() << "nb threads: " << thread_ids.size();
#endif

    if(last_messages.keys().contains(name)) {
        return last_messages[name];
    } else {
        return std::nullopt;
    }
}
