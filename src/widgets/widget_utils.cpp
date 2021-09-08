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
#include "plotter.h"
#include "link_status.h"


template<typename C, typename T>
QWidget* new_container(QWidget* parent) {
    return new C(
        [](QString ac_id, QWidget* container) {
            return new T(ac_id, container);
        },
        parent);
}

template<typename T>
QWidget* make_container(QString container, QWidget* parent) {
    if(container == "stack") {
        return new_container<StackContainer, T>(parent);
    } else if(container == "list") {
        return new_container<ListContainer, T>(parent);
    } else {
        throw runtime_error("unkown container " + container.toStdString());
    }
}


QWidget* makeWidget(QString name, QString container, QWidget* parent) {
    QWidget* widget = nullptr;

    if(name == "map2d") {
        widget = new PprzMap(parent);
    }
    else if (name == "PFD") {
        widget = new Pfd(parent);
    }
    else if (name == "settings") {
        widget = make_container<SettingsViewer>(container, parent);
    }
    else if (name == "strips") {
        widget = make_container<MiniStrip>(container, parent);
    }
    else if (name == "flightplan") {
        widget = make_container<FlightPlanViewerV2>(container, parent);
    }
    else if (name == "map_strip") {
        widget = make_container<Strip>(container, parent);
    }
    else if (name == "commands") {
        widget = make_container<Commands>(container, parent);
    }
    else if (name == "gps_classic_viewer") {
        widget = make_container<GPSClassicViewer>(container, parent);
    }
    else if (name == "flight_plan_editor") {
        widget = make_container<FlightPlanEditor>(container, parent);
    }
    else if (name == "plotter") {
        widget = make_container<Plotter>(container, parent);
    }
    else if (name == "link_status") {
        widget = make_container<LinkStatus>(container, parent);
    }
    else {
        std::string s = "Widget " + name.toStdString() + " unknown";
        throw runtime_error(s);
    }

    return widget;
}
