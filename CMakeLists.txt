cmake_minimum_required(VERSION 3.10)

project(PprzGCS)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fno-sized-deallocation")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


if(BUILD_TARGET STREQUAL "DEV_18_04")
    find_package(PROJ4 REQUIRED)
    get_filename_component(APP_DATA_PATH "./data/" ABSOLUTE)
elseif(BUILD_TARGET STREQUAL "BUILD_DEB_20_04")
    set(APP_DATA_PATH /usr/share/pprzgcs)
    set(PROJ4_LIBRARIES proj)
elseif(BUILD_TARGET STREQUAL "DEV_20_04")
    get_filename_component(APP_DATA_PATH "./data/" ABSOLUTE)
    set(PROJ4_LIBRARIES proj)
elseif(BUILD_TARGET STREQUAL "BUILD_DEB_18_04")
    set(APP_DATA_PATH /usr/share/pprzgcs)
elseif(BUILD_TARGET STREQUAL "BUILD_MAC")
    message(FATAL_ERROR "Building for MAC not yet supported!")
else()
    message(FATAL_ERROR "You must define BUILD_TARGET to one of: DEV_18_04, BUILD_DEB_20_04, BUILD_DEB_18_04, BUILD_MAC")
endif()



add_definitions(-DAPP_DATA_PATH="${APP_DATA_PATH}")

find_package(Boost COMPONENTS system)

find_package(Qt5 5.11 COMPONENTS Widgets Xml Network Svg REQUIRED)

find_package(pprzlink++ REQUIRED)

set(SOURCE
    ${SOURCE}
    src/main.cpp
    src/widgets/strip.cpp
    src/widgets/ac_selector.cpp
    src/widgets/settings_viewer.cpp
    src/widgets/pprzmap.cpp
    src/widgets/pfd.cpp
    src/widgets/strips.cpp
    src/widgets/mini_strip.cpp
    src/widgets/flightplan_viewerv2.cpp
    src/widgets/widget_stack.cpp
    src/widgets/layer_combo.cpp
    src/widgets/widget_utils.cpp
    src/widgets/pprzmap.ui
    src/widgets/commands.cpp
    src/widgets/gps_classic_viewer.cpp
    src/widgets/map/tileprovider.cpp
    src/widgets/map/tileproviderconfig.cpp
    src/widgets/map/tileitem.cpp
    src/widgets/map/map2d.cpp
    src/widgets/map/mapwidget.cpp
    src/widgets/map/maplayercontrol.cpp
    src/widgets/map/layertab.cpp
    src/widgets/map/mapscene.cpp
    src/widgets/map/maputils.cpp
    src/widgets/map/waypointeditor.cpp
    src/widgets/map/acitemmanager.cpp

    src/widgets/map/map_items/map_item.cpp
    src/widgets/map/map_items/waypoint_item.cpp
    src/widgets/map/map_items/path_item.cpp
    src/widgets/map/map_items/circle_item.cpp
    src/widgets/map/map_items/aircraft_item.cpp

    src/widgets/map/graphics_objects/graphics_object.cpp
    src/widgets/map/graphics_objects/graphics_point.cpp
    src/widgets/map/graphics_objects/graphics_line.cpp
    src/widgets/map/graphics_objects/graphics_circle.cpp
    src/widgets/map/graphics_objects/graphics_aircraft.cpp
    src/widgets/map/graphics_objects/graphics_track.cpp
    src/widgets/map/graphics_objects/graphics_text.cpp

    # src/widgets/map/fpedit_statemachines/item_edit_state_machine.cpp
    # src/widgets/map/fpedit_statemachines/waypointitem_sm.cpp
    # src/widgets/map/fpedit_statemachines/circleitem_sm.cpp
    # src/widgets/map/fpedit_statemachines/pathitem_sm.cpp


    src/widgets/basics/imagebutton.cpp
    src/widgets/basics/double_slider.cpp
    src/widgets/basics/switch.cpp
    src/widgets/basics/graphlabel.cpp
    src/widgets/basics/jaugelabel.cpp
    src/widgets/basics/colorlabel.cpp
    src/widgets/basics/flowlayout.cpp
    src/widgets/basics/mbpushbutton.cpp
    src/widgets/basics/lock_button.cpp

    src/layout_builder.cpp
    src/pprz_dispatcher.cpp
    src/pprzmain.cpp
    src/dispatcher_ui.cpp
    src/geometry/coordinatestransform.cpp
    src/geometry/point2dlatlon.cpp
    src/geometry/point2dtile.cpp
    src/geometry/point2dpseudomercator.cpp
    resources/ressources.qrc
    src/AircraftManager.cpp
    src/srtm_manager.cpp
    src/aircraft.cpp
    src/configure.cpp
    src/common/waypoint.cpp
    src/common/sector.cpp
    src/common/block.cpp
    src/common/flightplan.cpp
    src/common/setting_menu.cpp
    src/common/setting.cpp
    src/common/airframe.cpp
    src/common/aircraft_status.cpp
    src/common/gcs_utils.cpp
    src/common/units.cpp
)

include_directories(
    src/
    src/common/
    src/widgets/
    src/geometry/
    src/widgets/map/
    src/widgets/map/map_items/
    src/widgets/map/fpedit_statemachines/
    src/widgets/map/graphics_objects/
    src/widgets/basics/
)

include_directories(${CMAKE_PREFIX_PATH}/include)
link_directories(${CMAKE_PREFIX_PATH}/lib)

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME}
    pprzlink++
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    Qt5::Svg
    ${PROJ4_LIBRARIES}
    ${Boost_LIBRARIES}
    zip
)


