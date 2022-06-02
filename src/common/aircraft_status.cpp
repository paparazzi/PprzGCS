#include "aircraft_status.h"
#include <QDebug>
#include "coordinatestransform.h"
#include "AircraftManager.h"

AircraftStatus::AircraftStatus(QString ac_id, QObject *parent) : QObject(parent),
    ac_id(ac_id)
{
    watcher = new AircraftWatcher(ac_id, this);

    //listen for NAVIGATION_REF to update origin waypoint of fixedwings
    PprzDispatcher::get()->bind("NAVIGATION_REF", this, [=](QString sender, pprzlink::Message msg) {
        if(sender == ac_id) {
            int32_t utm_east, utm_north;
            uint8_t utm_zone;
            float ground_alt;
            msg.getField("utm_east", utm_east);
            msg.getField("utm_north", utm_north);
            msg.getField("utm_zone", utm_zone);
            msg.getField("ground_alt", ground_alt);

            auto latlon = CoordinatesTransform::get()->utm_to_wgs84(utm_east, utm_north, utm_zone, true);
            auto orig = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getOrigin();
            orig->setLat(latlon.lat());
            orig->setLon(latlon.lon());
        }
    });

    //listen for INS_REF to update origin waypoint of rotorcrafts
    PprzDispatcher::get()->bind("INS_REF", this, [=](QString sender, pprzlink::Message msg) {
        if(sender == ac_id) {

            int32_t lat0, lon0, alt0;
            msg.getField("lat0", lat0);
            msg.getField("lon0", lon0);
            msg.getField("alt0", alt0);

            auto orig = AircraftManager::get()->getAircraft(ac_id)->getFlightPlan()->getOrigin();
            orig->setLat(lat0/1e7);
            orig->setLon(lon0/1e7);
            orig->setAlt(alt0/1e3);
        }
    });

}

void AircraftStatus::updateMessage(pprzlink::Message msg) {
    QString id;
    msg.getField("ac_id", id);
    if(id == ac_id) {
        auto name = msg.getDefinition().getName();
        last_messages[name] = msg;

        if(name == "FLIGHT_PARAM") {
            emit flight_param();
        }
        else if(name == "AP_STATUS") {
            emit ap_status();
        }
        else if(name == "NAV_STATUS") {
            emit nav_status();
        }
        else if(name == "CIRCLE_STATUS") {
            emit circle_status();
        }
        else if(name == "SEGMENT_STATUS") {
            emit segment_status();
        }
        else if(name == "ENGINE_STATUS") {
            emit engine_status();
            watcher->watch_bat(msg);
        }
        else if(name == "WAYPOINT_MOVED") {
            emit waypoint_moved();
        }
        else if(name == "DL_VALUES") {
            emit dl_values();
        }
        else if(name == "TELEMETRY_STATUS") {
            QString link_id;
            msg.getField("link_id", link_id);

            if(telemetry_messages.contains("no_id") && link_id != "no_id") {
                telemetry_messages.remove("no_id");
            }

            telemetry_messages[link_id] = msg;
            emit telemetry_status();
            watcher->watch_links(msg);
        }
        else if(name == "FLY_BY_WIRE") {
            emit fly_by_wire();
        }
        else if(name == "SVSINFO") {
            emit svsinfo();
        }
    }
}

std::optional<pprzlink::Message> AircraftStatus::getMessage(QString name) {
    if(name == "TELEMETRY_STATUS") {
        throw std::runtime_error("TELEMETRY_STATUS messages should be handled via the specialized methods!");
        return std::nullopt;
    }
    if(last_messages.contains(name)) {
        return last_messages[name];
    } else {
        return std::nullopt;
    }
}
