#include "PprzApplication.h"
#include "PprzToolbox.h"
#include "pprzmain.h"

PprzApplication* PprzApplication::_app = nullptr;

PprzApplication::PprzApplication(int &argc, char **argv, int flags) : QApplication(argc, argv, flags)
{
    _app = this;
    _toolbox = new PprzToolbox(this);
    //_toolbox->setChildToolboxes();
    _mainWindow = new PprzMain();
}

/// @brief Returns the PprzApplication object singleton.
PprzApplication* pprzApp(void)
{
    return PprzApplication::_app;
}

void PprzApplication::init() {
    _toolbox->setChildToolboxes();
}
