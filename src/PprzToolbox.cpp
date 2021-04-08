#include "PprzToolbox.h"
#include "AircraftManager.h"
#include "srtm_manager.h"
#include "pprz_dispatcher.h"

PprzToolbox::PprzToolbox(PprzApplication* app)
{
    _aircraftManager = new AircraftManager(app, this);
    _srtmManager     = new SRTMManager    (app, this);
    _pprzDispatcher  = new PprzDispatcher (app, this);
}

void PprzToolbox::setChildToolboxes(void) {
    _aircraftManager->setToolbox(this);
    _srtmManager->setToolbox(this);
    _pprzDispatcher->setToolbox(this);
}


PprzTool::PprzTool(PprzApplication* app, PprzToolbox* toolbox) : _app(app), _toolbox(nullptr)
{
    (void)toolbox;
}
void PprzTool::setToolbox(PprzToolbox* toolbox)
{
    _toolbox = toolbox;
}
