cmake_minimum_required(VERSION 3.12)

project(PprzGCS)

set(Qt5_DIR "$HOME/Qt/5.12.5/gcc_64/lib/cmake/Qt5")

#set(CMAKE_PREFIX_PATH  "${CMAKE_PREFIX_PATH};/home/fabien/local/lib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()


find_package(Qt5 COMPONENTS Widgets Xml Network Svg REQUIRED)
find_package(PROJ REQUIRED)
find_package(libzippp 3.0 REQUIRED)

set(PPRZLINK_PATH ext/pprzlink/lib/v2.0/C++)
add_subdirectory(${PPRZLINK_PATH} pprzlink)

#set(LIBZIPPP_PATH ext/libzippp)
#add_subdirectory(${LIBZIPPP_PATH} pprzlink)

set(SOURCE
    ${SOURCE}
    src/main.cpp
    src/widgets/strip.cpp
    src/widgets/ac_selector.cpp
    src/widgets/settings_viewer.cpp
    src/widgets/pprzmap.cpp
    src/widgets/pfd.cpp
    src/widgets/strips.cpp
    src/widgets/flightplan_viewer.cpp
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

    src/widgets/map/fpedit_statemachines/item_edit_state_machine.cpp
    src/widgets/map/fpedit_statemachines/waypointitem_sm.cpp
    src/widgets/map/fpedit_statemachines/circleitem_sm.cpp
    src/widgets/map/fpedit_statemachines/pathitem_sm.cpp


    src/widgets/basics/imagebutton.cpp
    src/widgets/basics/double_slider.cpp
    src/widgets/basics/switch.cpp
    src/widgets/basics/graphlabel.cpp
    src/widgets/basics/jaugelabel.cpp
    src/widgets/basics/colorlabel.cpp
    src/widgets/basics/flowlayout.cpp
    src/widgets/basics/mbpushbutton.cpp
    src/widgets/basics/sidepanel.cpp
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
    src/common/block.cpp
    src/common/flightplan.cpp
    src/common/setting_menu.cpp
    src/common/setting.cpp
    src/common/airframe.cpp
    src/common/aircraft_status.cpp
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
    ${PPRZLINK_PATH}
    ${PROJ_INCLUDE_DIRS} 
)

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME}
    pprzlink++
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    Qt5::Svg
    ${PROJ_LIBRARIES}
    ${Boost_LIBRARIES}
    libzippp::libzippp
)

