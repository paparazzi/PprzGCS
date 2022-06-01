#include "watcher.h"
#include "pprz_dispatcher.h"
#include "AircraftManager.h"

Watcher::Watcher(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void Watcher::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
    connect(_toolbox->pprzDispatcher(), &PprzDispatcher::engine_status, this, &Watcher::bat_watcher);
}


void Watcher::bat_watcher(pprzlink::Message msg) {
    QString ac_id;
    float bat;
    msg.getField("ac_id", ac_id);
    msg.getField("bat", bat);
    // if the AC is not yet known, fetch bat params from the airframe
    if(!ac_bat_params.contains(ac_id)) {
        auto airframe = _toolbox->aircraftManager()->getAircraft(ac_id)->getAirframe();
        auto lbl = airframe->getDefine("LOW_BAT_LEVEL");
        auto cbl = airframe->getDefine("CRITIC_BAT_LEVEL");
        auto ctbl = airframe->getDefine("CATASTROPHIC_BAT_LEVEL");

        float low = lbl.has_value() ? lbl->value.toDouble() : 10.5;
        float critic = cbl.has_value() ? cbl->value.toDouble() : 10.;
        float catastrophic = ctbl.has_value() ? ctbl->value.toDouble() : 9.;

        ac_bat_params[ac_id] = {low, critic, catastrophic};
    }

    BatStatus bs;
    if(bat < ac_bat_params[ac_id].catastrophic) {
        bs = BatStatus::CATASTROPHIC;
    } else if(bat < ac_bat_params[ac_id].critic) {
        bs = BatStatus::CRITIC;
    } else if(bat < ac_bat_params[ac_id].low) {
        bs = BatStatus::LOW;
    } else {
        bs = BatStatus::OK;
    }
    emit bat_status(ac_id, bs);
}
