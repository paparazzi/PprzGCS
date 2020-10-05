#include "settingsviewers_stack.h"
#include "dispatcher_ui.h"
#include "AircraftManager.h"


SettingsViewersStack::SettingsViewersStack(QWidget *parent) : QWidget(parent)
{
    vLayout = new QVBoxLayout(this);
    stack = new QStackedWidget(this);

    ac_selector = new ACSelector(this);
    vLayout->addWidget(ac_selector);



    vLayout->addWidget(stack);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &SettingsViewersStack::handleNewAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this,
            [=](QString ac_id) {
                if(viewers_indexes.keys().contains(ac_id)) {
                  stack->setCurrentIndex(viewers_indexes[ac_id]);
                }
            });

    stack->setAutoFillBackground(true);
}

void SettingsViewersStack::handleNewAC(QString ac_id) {
    auto sv = new SettingsViewer(this);
    sv->init(ac_id);
    int index = stack->addWidget(sv);
    viewers_indexes[ac_id] = index;
}
