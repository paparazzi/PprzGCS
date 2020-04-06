cmake_minimum_required(VERSION 2.8)

project(PprzGCS)

set(Qt5_DIR "$HOME/Qt/5.12.0/gcc_64/lib/cmake/Qt5")

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


set(PPRZLINK_PATH ext/pprzlink/lib/v2.0/C++)
add_subdirectory(${PPRZLINK_PATH} pprzlink)


set(SOURCE
    ${SOURCE}
    src/main.cpp
    src/widgets/strip.ui
    src/widgets/strip.cpp
    src/widgets/ac_selector.ui
    src/widgets/ac_selector.cpp
    src/widgets/settings_explorer.ui
    src/widgets/settings_explorer.cpp
    src/widgets/pprzmap.cpp

    src/widgets/pprzmap.ui
    src/widgets/map/tileprovider.cpp
    src/widgets/map/tileproviderconfig.cpp
    src/widgets/map/tileitem.cpp
    src/widgets/map/map2d.cpp
    src/widgets/map/mapwidget.cpp
    src/widgets/map/maplayercontrol.cpp
    src/widgets/map/layertab.cpp
    src/widgets/map/mapscene.cpp
    src/widgets/map/maputils.cpp
    src/widgets/map/mapstrip.cpp

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
    src/layout_builder.cpp
    src/pprz_dispatcher.cpp
    src/pprzmain.cpp
    src/dispatcher_ui.cpp
    src/geometry/coordinatestransform.cpp
    src/geometry/point2dlatlon.cpp
    src/geometry/point2dtile.cpp
    resources/ressources.qrc
    src/AircraftManager.cpp
    src/aircraft.cpp
    src/common/waypoint.cpp
    src/common/block.cpp
    src/common/flightplan.cpp
    src/common/setting_menu.cpp
    src/common/setting.cpp
    src/common/airframe.cpp
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
)

