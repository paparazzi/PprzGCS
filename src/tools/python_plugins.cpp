#include "python_plugins.h"
#include "pprz_dispatcher.h"
#include "AircraftManager.h"
#include "pythonutils.h"

PythonPlugins::PythonPlugins(PprzApplication* app, PprzToolbox* toolbox) : PprzTool(app, toolbox)
{

}

void PythonPlugins::setToolbox(PprzToolbox* toolbox) {
    PprzTool::setToolbox(toolbox);
}

void PythonPlugins::bind_main_window(PprzMain* w) {
    if (!PythonUtils::bindAppObject("__main__", "mainWindow", PythonUtils::MainWindowType, w)) {
        qDebug() << "Error loading the application module";
    }
}

void PythonPlugins::printDiagnostics()
{
    const QStringList script = QStringList()
            << "import sys" << "print('Path=', sys.path)" << "print('Executable=', sys.executable)";
    runScript(script);
}

void PythonPlugins::runScript(const QStringList &script)
{
    if (!::PythonUtils::runScript(script)) {
        qDebug() << "Error running script";
    }
}
