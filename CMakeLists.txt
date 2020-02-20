cmake_minimum_required(VERSION 2.8)

project(PprzGCS)

set(LIB_PPRZLINK "/home/fabien/local/lib")
set(INCLUDE_PPRZLINK "/home/fabien/local/include/pprzlink")

set(CMAKE_PREFIX_PATH  "${CMAKE_PREFIX_PATH};/home/fabien/local/lib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Xml Network REQUIRED)
find_package(tinyxml2 REQUIRED )
find_package(Boost COMPONENTS system)

find_library(PPRZLINK_LIB NAMES pprzlink++ PATH ${LIB_PPRZLINK})
find_library(IVY_LIB NAMES ivy)


set(SOURCE
    ${SOURCE}
    src/main.cpp
    src/widgets/strip.ui
    src/widgets/strip.cpp
    src/widgets/ac_selector.ui
    src/widgets/ac_selector.cpp
    src/widgets/pprzmap.cpp
    src/widgets/map/mapitem.cpp
    src/widgets/pprzmap.ui
    src/widgets/map/tileprovider.cpp
    src/widgets/map/tileproviderconfig.cpp
    src/widgets/map/tileitem.cpp
    src/widgets/map/map2d.cpp
    src/widgets/map/mapwidget.cpp
    src/widgets/map/maplayercontrol.cpp
    src/widgets/map/layertab.cpp
    src/widgets/map/mapscene.cpp
    src/widgets/map/waypointitem.cpp
    src/widgets/map/maputils.cpp
    src/widgets/map/graphicspoint.cpp
    src/widgets/map/graphicsobject.cpp
    src/widgets/map/path.cpp
    src/widgets/map/graphicsline.cpp
    src/widgets/map/graphicscircle.cpp
    src/widgets/map/circleitem.cpp
    src/widgets/basics/imagebutton.cpp
    src/layout_builder.cpp
    src/pprz_dispatcher.cpp
    src/pprzmain.cpp
    src/dispatcher_ui.cpp
    src/geometry/coordinates.cpp
    src/geometry/point2dlatlon.cpp
    src/geometry/point2dtile.cpp
    resources/ressources.qrc
)

include_directories(
    src/
    src/widgets/
    src/geometry/
    src/widgets/map/
    src/widgets/basics/
    ${INCLUDE_PPRZLINK}
)

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    ${Boost_LIBRARIES}
    ${PPRZLINK_LIB}
    ${IVY_LIB}
    tinyxml2
    Boost::system
)

