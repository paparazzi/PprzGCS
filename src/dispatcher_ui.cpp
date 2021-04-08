#include "dispatcher_ui.h"

DispatcherUi::DispatcherUi(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{
    connect(this, &DispatcherUi::ac_selected, [=](QString ac_id){
        selected_ac_id = ac_id;
    });

}
