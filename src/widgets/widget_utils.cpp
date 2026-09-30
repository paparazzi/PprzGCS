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
#include "tuple_helpers.h"
#include "gvf_viewer.h"
#include "grid_viewer.h"
#include "group_controller.h"

#include "chat.h"
#include "checklist.h"

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>

#include <exception>

using ac_widgets_list = std::tuple<
    SettingsViewer, MiniStrip,
    Commands, FlightPlanViewerV2,
    GPSClassicViewer, FlightPlanEditor,
    Plotter, LinkStatus, GVFViewer, GridViewer,
    Checklist
>;
using simple_widgets_list = std::tuple<PprzMap, Pfd, Chat, GroupController>;
using containers_list = std::tuple<StackContainer, ListContainer>;

std::map<QString, size_t> AC_WIDGETS_MAP = {
    {"settings", tuple_element_index_v<SettingsViewer, ac_widgets_list>},
    {"strips", tuple_element_index_v<MiniStrip, ac_widgets_list>},
    {"commands", tuple_element_index_v<Commands, ac_widgets_list>},
    {"flightplan", tuple_element_index_v<FlightPlanViewerV2, ac_widgets_list>},
    {"gps_classic_viewer", tuple_element_index_v<GPSClassicViewer, ac_widgets_list>},
    {"flight_plan_editor", tuple_element_index_v<FlightPlanEditor, ac_widgets_list>},
    {"plotter", tuple_element_index_v<Plotter, ac_widgets_list>},
    {"link_status", tuple_element_index_v<LinkStatus, ac_widgets_list>},
    {"gvf_viewer", tuple_element_index_v<GVFViewer, ac_widgets_list>},
    {"checklist", tuple_element_index_v<Checklist, ac_widgets_list>},
    {"grid_viewer", tuple_element_index_v<GridViewer, ac_widgets_list>},
};


std::map<QString, size_t> SIMPLE_WIDGETS_MAP = {
    {"map2d", tuple_element_index_v<PprzMap, simple_widgets_list>},
    {"PFD", tuple_element_index_v<Pfd, simple_widgets_list>},
    {"chat", tuple_element_index_v<Chat, simple_widgets_list>},
    {"group_controller", tuple_element_index_v<GroupController, simple_widgets_list>},
};



std::map<QString, size_t> CONTAINER_WIDGETS_MAP = {
    {"stack", tuple_element_index_v<StackContainer, containers_list>},
    {"list", tuple_element_index_v<ListContainer, containers_list>},
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

static QWidget* makeErrorWidget(QWidget* parent, const QString& message) {
    qCritical().noquote() << message;
    auto widget = new QWidget(parent);
    auto layout = new QVBoxLayout(widget);
    auto label = new QLabel(message, widget);
    label->setWordWrap(true);
    layout->addWidget(label);
    layout->addStretch();
    return widget;
}

template<class ET>
QWidget* createInstance(const ET eventType, QWidget* parent) {
    try {
        return select_type<QWidget*, simple_widgets_list>([&](auto p){
            return new std::decay_t<decltype(*p)>(parent);
        }, (std::size_t)eventType);
    } catch (const std::exception& e) {
        return makeErrorWidget(parent, QString::fromUtf8(e.what()));
    } catch (...) {
        return makeErrorWidget(parent, QStringLiteral("Unknown error creating widget"));
    }
}


template<class ET>
auto getConstructor(const ET eventType) {
    return select_type<std::function<QWidget*(QString, QWidget*)>, ac_widgets_list>([&](auto p){
        return [](QString ac_id, QWidget* container) {
            return new std::decay_t<decltype(*p)>(ac_id, container);
        };
    }, (std::size_t)eventType);
}


template<class ET, class... X>
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
    try {
        auto ac_widget_index = AC_WIDGETS_MAP.find(name);
        auto alt_index = AC_WIDGETS_MAP.find(alt);
        auto simple_widget_index = SIMPLE_WIDGETS_MAP.find(name);

        if(ac_widget_index != AC_WIDGETS_MAP.end()) {
            if(alt_index != AC_WIDGETS_MAP.end()) {
                return make_container(container, parent, ac_widget_index->second, alt_index->second);
            }
            return make_container(container, parent, ac_widget_index->second);
        }

        if(simple_widget_index != SIMPLE_WIDGETS_MAP.end()) {
            return createInstance(simple_widget_index->second, parent);
        }

        throw std::runtime_error("Widget " + name.toStdString() + " unknown");
    } catch (const std::exception& e) {
        return makeErrorWidget(parent, QString::fromUtf8(e.what()));
    } catch (...) {
        return makeErrorWidget(parent, QStringLiteral("Unknown error creating widget %1").arg(name));
    }
}
