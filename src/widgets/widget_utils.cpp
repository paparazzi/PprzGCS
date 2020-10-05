#include "widget_utils.h"
#include "strips.h"
#include "ac_selector.h"
#include "pprzmap.h"
#include "pfd.h"
#include "flightplan_viewer.h"
#include "layer_combo.h"
#include "settingsviewers_stack.h"

QWidget* makeWidget(QString name, QWidget* parent) {
    QWidget* widget = nullptr;

    if (name == "strips") {
        widget = new Strips(parent);
    } else if (name == "alarms") {
        widget = new ACSelector(parent);
    } else if(name == "map2d") {
        widget = new PprzMap(parent);
    } else if (name == "aircraft" or name=="altgraph") {
        widget = new QWidget(); // dummy widget
    } else if (name == "settings") {
        widget = new SettingsViewersStack(parent);
    } else if (name == "PFD") {
        widget = new Pfd(parent);
    } else if (name == "flight_plan") {
        widget = new FlightPlanViewer(parent);
    }
    else {
        std::string s = "Widget " + name.toStdString() + " unknown";
        throw runtime_error(s);
    }

    return widget;
}
