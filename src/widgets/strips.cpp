#include "strips.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "strip.h"

Strips::Strips(QWidget *parent) : QStackedWidget(parent)
{
    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &Strips::handleNewAC);

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
        [=](QString ac_id) {
        for(auto ids: ac_ids) {
            if(ids.first == ac_id) {
                setCurrentIndex(ids.second);
            }
        }
    });
}

void Strips::handleNewAC(QString ac_id) {
    auto ac = AircraftManager::get()->getAircraft(ac_id);
    ac_ids[ac_id] = count();


    //AircraftManager::get()->getAircraft(ac_id);
    QWidget* pageWidget = new Strip(ac_id, this);

    addWidget(pageWidget);
    setCurrentWidget(pageWidget);
}
