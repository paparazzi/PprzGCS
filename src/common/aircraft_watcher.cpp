#include "aircraft_watcher.h"
#include "AircraftManager.h"

AircraftWatcher::AircraftWatcher(QString ac_id, QObject *parent) : QObject(parent),
    ac_id(ac_id)
{
}

void AircraftWatcher::init() {
    auto airframe = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id)->getAirframe();
    // not set: 3.5, 3.25 and 3.0 V per cell
    bat_low = airframe->batteryLevel("LOW_BAT_LEVEL");
    bat_critic = airframe->batteryLevel("CRITIC_BAT_LEVEL");
    bat_catastrophic = airframe->batteryLevel("CATASTROPHIC_BAT_LEVEL");
}

void AircraftWatcher::watch_bat(pprzlink::Message msg) {
    float bat;
    msg.getField("bat", bat);

    BatStatus bs;
    if(bat < bat_catastrophic) {
        bs = BatStatus::CATASTROPHIC;
    } else if(bat < bat_critic) {
        bs = BatStatus::CRITIC;
    } else if(bat < bat_low) {
        bs = BatStatus::LOW;
    } else {
        bs = BatStatus::OK;
    }

    emit bat_status(bs);
}

void AircraftWatcher::watch_links(pprzlink::Message msg) {
    QString link_id;
    float time_since_last_msg;
    msg.getField("link_id", link_id);
    msg.getField("time_since_last_msg", time_since_last_msg);
    link_times[link_id] = time_since_last_msg;

    int nb_link_losts = std::accumulate(link_times.begin(), link_times.end(), 0,
        [](int n, auto t){
            if(t>5) { ++n; }
            return n;
        }
    );

    LinkStatus status;
    if(nb_link_losts == 0) {
        status = LinkStatus::LINK_OK;
    } else if (nb_link_losts == link_times.size()) {
        status = LinkStatus::LINK_LOST;
    } else {
        status = LinkStatus::LINK_PARTIALY_LOST;
    }

    emit link_status(status);
}
