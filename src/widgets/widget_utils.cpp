#include "widget_utils.h"
#include "listcontainer.h"
#include "ac_selector.h"
#include "pprzmap.h"
#include "pfd.h"
#include "layer_combo.h"
#include "settings_viewer.h"
#include "stackcontainer.h"
#include "flightplan_viewerv2.h"
#include "commands.h"
#include "gps_classic_viewer.h"
#include "flightplaneditor.h"
#include "mini_strip.h"

template<typename T>
QWidget* make_stack(QWidget* parent) {
    return new StackContainer(
        [](QString ac_id, QWidget* container) {
            return new T(ac_id, container);
        },
        parent);
}

template<typename T>
QWidget* make_list(QWidget* parent) {
    return new ListContainer(
        [](QString ac_id, QWidget* container) {
            return new T(ac_id, container);
        },
        parent);
}

QWidget* makeWidget(QString name, QWidget* parent) {
    QWidget* widget = nullptr;

    if (name == "strips") {
        widget = make_list<MiniStrip>(parent);
    } else if (name == "alarms") {
        widget = new ACSelector(parent);
    } else if(name == "map2d") {
        widget = new PprzMap(parent);
    } else if (name == "aircraft" or name=="altgraph") {
        widget = new QWidget(); // dummy widget
    } else if (name == "settings") {
        widget = make_stack<SettingsViewer>(parent);
    } else if (name == "PFD") {
        widget = new Pfd(parent);
    }
    else if (name == "flight_planV2") {
        widget = make_stack<FlightPlanViewerV2>(parent);
    }
    else if (name == "map_strip") {
        widget = make_stack<Strip>(parent);
    }
    else if (name == "commands") {
        widget = make_stack<Commands>(parent);
    }
    else if (name == "gps_classic_viewer") {
        widget = make_stack<GPSClassicViewer>(parent);
    }
    else if (name == "flight_plan_editor") {
        widget = make_stack<FlightPlanEditor>(parent);
    }
    else {
        std::string s = "Widget " + name.toStdString() + " unknown";
        throw runtime_error(s);
    }

    return widget;
}
