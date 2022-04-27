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

using ac_widgets_list = std::tuple<
    SettingsViewer, MiniStrip,
    Commands, FlightPlanViewerV2,
    GPSClassicViewer, FlightPlanEditor,
    Plotter, LinkStatus
>;

enum ACW {
    SETTINGS_VIEWER,
    MINISTRIP,
    COMMANDS,
    FLIGHTPLAN,
    GPS,
    FPEDITOR,
    PLOTTER,
    LINKSTATUS
};


std::map<QString, size_t> AC_WIDGETS_MAP = {
    {"settings", ACW::SETTINGS_VIEWER},
    {"strips", ACW::MINISTRIP},
    {"commands", ACW::COMMANDS},
    {"flightplan", ACW::FLIGHTPLAN},
    {"gps_classic_viewer", ACW::GPS},
    {"flight_plan_editor", ACW::FPEDITOR},
    {"plotter", ACW::PLOTTER},
    {"link_status", ACW::LINKSTATUS},
};


using simple_widgets_list = std::tuple<PprzMap, Pfd>;

enum SW {
    MAP,
    PFD
};

std::map<QString, size_t> SIMPLE_WIDGETS_MAP = {
    {"map2d", SW::MAP},
    {"PFD", SW::PFD},
};

using containers_list = std::tuple<StackContainer, ListContainer>;

enum CW {
    STACK,
    LIST,
};

std::map<QString, size_t> CONTAINER_WIDGETS_MAP = {
    {"stack", CW::STACK},
    {"list", CW::LIST},
};

// the following code is inspired by https://codereview.stackexchange.com/a/166731

template <std::size_t... Is, class F>
static inline void static_for_impl(F&& f, std::index_sequence<Is...>) {
    (f(std::integral_constant<std::size_t, Is>()), ...);
}
template <std::size_t N, class F>
void static_for(F f) {
    static_for_impl(f, std::make_index_sequence<N>());
}

template <class tuple, class F>
void for_all_types(F f) {
    static_for<std::tuple_size_v<tuple>>([&](auto N){
        using T = std::tuple_element_t<N, tuple>;
        if constexpr (!std::is_same_v<void, T>)
            f((T*)0, N);
    });
}

template <class R, class tuple, class F>
R select_type(F f, std::size_t i) {
    R r;
    bool found = false;
    for_all_types<tuple>([&](auto p, auto N){
        if (i == N) {
            r = f(p);
            found = true;
        }
    });
    if (!found)
        throw std::invalid_argument("");
    return r;
}



template<typename tutu, class... X, class ET>
auto createInstance(const ET eventType, X&&... x) {
    return select_type<QWidget*, tutu>([&](auto p){
        return new std::decay_t<decltype(*p)>(std::forward<X>(x)...);
    }, (std::size_t)eventType);
}


template<class ET>
auto getConstructor(const ET eventType) {
    return select_type<std::function<QWidget*(QString, QWidget*)>, ac_widgets_list>([&](auto p){
        return [](QString ac_id, QWidget* container) {
            return new std::decay_t<decltype(*p)>(ac_id, container);
        };
    }, (std::size_t)eventType);
}


template<class... X, class ET>
auto createContainer(const ET eventType, X&&... x) {
    return select_type<QWidget*, containers_list>([&](auto p){
        return new std::decay_t<decltype(*p)>(std::forward<X>(x)...);
    }, (std::size_t)eventType);
}


QWidget* make_container(QString container, QWidget* parent, size_t wt) {
    auto container_index = CONTAINER_WIDGETS_MAP.find(container);
    if(container_index != CONTAINER_WIDGETS_MAP.end()) {
        return createContainer(container_index->second, getConstructor(wt), parent);
    } else {
        throw runtime_error("unkown container " + container.toStdString());
    }
}

QWidget* make_container(QString container, QWidget* parent, size_t wt, size_t wt2) {
    auto container_index = CONTAINER_WIDGETS_MAP.find(container);
    if(container_index != CONTAINER_WIDGETS_MAP.end()) {
        return createContainer(container_index->second, getConstructor(wt), getConstructor(wt2), parent);
    } else {
        throw runtime_error("unkown container " + container.toStdString());
    }
}


QWidget* makeWidget(QWidget* parent, QString container, QString name, QString alt) {
    QWidget* widget = nullptr;

    auto ac_widget_index = AC_WIDGETS_MAP.find(name);
    auto alt_index = AC_WIDGETS_MAP.find(alt);
    auto simple_widget_index = SIMPLE_WIDGETS_MAP.find(name);

    if(ac_widget_index != AC_WIDGETS_MAP.end()) {
        if(alt_index != AC_WIDGETS_MAP.end()) {
            widget = make_container(container, parent, ac_widget_index->second, alt_index->second);
        } else {
            widget = make_container(container, parent, ac_widget_index->second);
        }

    }
    else if(simple_widget_index != SIMPLE_WIDGETS_MAP.end()) {
        widget = createInstance<simple_widgets_list>(simple_widget_index->second, parent);
    }
    else {
        std::string s = "Widget " + name.toStdString() + " unknown";
        throw runtime_error(s);
    }

    return widget;
}
