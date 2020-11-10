#include "dispatcher_ui.h"


DispatcherUi* DispatcherUi::singleton = nullptr;

DispatcherUi::DispatcherUi(QObject *parent) : QObject(parent)
{
    connect(this, &DispatcherUi::ac_selected, [=](QString ac_id){
        selected_ac_id = ac_id;
    });

}
