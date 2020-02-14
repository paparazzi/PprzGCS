cmake_minimum_required(VERSION 2.8)

project(PprzGCS)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Xml Network REQUIRED)

# LibProj dependency for calculating coordinate projections
#find_package(PROJ4 REQUIRED proj>=4.0)
#if (PROJ4_FOUND)
#    include_directories(${PROJ4_INCLUDE_DIRS})
#    link_directories(${PROJ4_LIBRARY_DIRS})
#endif (PROJ4_FOUND)


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
)

#set(HEADERS
#    ${HEADERS}
#    src/widgets/strip.h
#    src/layout_builder.h
#    src/pprz_dispatcher.h
#    src/pprzmain.h
#    src/dispatcher_ui.h
#    src/gcs.h
#    src/uisignals.h
#    src/widgets/ac_selector.h
#    src/widgets/map2d.h
#    src/widgets/strip.h
#    )


add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(PprzGCS Qt5::Widgets Qt5::Xml Qt5::Network ${PROJ4_LIBRARIES} ${Boost_LIBRARIES})

