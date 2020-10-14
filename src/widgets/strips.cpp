#include "strips.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"
#include "strip.h"

Strips::Strips(QWidget *parent) : QWidget(parent)
{
    new QVBoxLayout(this);
    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &Strips::handleNewAC);

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
        [=](QString ac_id) {
        for(auto strip: strips) {
            if(strip.first == ac_id) {
                strip.second->show();
            } else {
                strip.second->hide();
            }
        }
    });
}

void Strips::handleNewAC(QString ac_id) {
    auto ac = AircraftManager::get()->getAircraft(ac_id);

    QWidget* pageWidget = new Strip(ac_id, this);
    strips[ac_id] = pageWidget;

    layout()->addWidget(pageWidget);
}
