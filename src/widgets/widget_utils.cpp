#include "widget_utils.h"
#include "strips.h"
#include "ac_selector.h"
#include "pprzmap.h"
#include "pfd.h"
#include "flightplan_viewer.h"
#include "layer_combo.h"
#include "settings_viewer.h"
#include "widget_stack.h"
#include "flightplan_viewerv2.h"
#include "commands.h"

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
        widget = new WidgetStack(
                [](QString ac_id, QWidget* container) {
                    return new SettingsViewer(ac_id, container);
                },
                parent);
    } else if (name == "PFD") {
        widget = new Pfd(parent);
    } else if (name == "flight_plan") {
        widget = new FlightPlanViewer(parent);
    }
    else if (name == "flight_planV2") {
        widget = new WidgetStack(
                [](QString ac_id, QWidget* container) {
                    return new FlightPlanViewerV2(ac_id, container);
                },
                parent);
    }
    else if (name == "map_strip") {
        widget = new WidgetStack(
                [](QString ac_id, QWidget* container) {
                    return new Strip(ac_id, container, true);
                },
                parent);
    }
    else if (name == "commands") {
        widget = new WidgetStack(
                [](QString ac_id, QWidget* container) {
                    return new Commands(ac_id, container);
                },
                parent, false);
    }
    else {
        std::string s = "Widget " + name.toStdString() + " unknown";
        throw runtime_error(s);
    }

    return widget;
}
