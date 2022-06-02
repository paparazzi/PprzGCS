#include "aircraft_watcher.h"
#include "AircraftManager.h"

AircraftWatcher::AircraftWatcher(QString ac_id, QObject *parent) : QObject(parent),
    ac_id(ac_id)
{
}

void AircraftWatcher::init() {
    auto airframe = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id)->getAirframe();
    auto lbl = airframe->getDefine("LOW_BAT_LEVEL");
    auto cbl = airframe->getDefine("CRITIC_BAT_LEVEL");
    auto ctbl = airframe->getDefine("CATASTROPHIC_BAT_LEVEL");

    bat_low = lbl.has_value() ? lbl->value.toDouble() : 10.5;
    bat_critic = cbl.has_value() ? cbl->value.toDouble() : 10.;
    bat_catastrophic = ctbl.has_value() ? ctbl->value.toDouble() : 9.;
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
