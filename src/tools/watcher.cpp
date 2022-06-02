#include "watcher.h"
#include "pprz_dispatcher.h"
#include "AircraftManager.h"

Watcher::Watcher(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void Watcher::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
}

