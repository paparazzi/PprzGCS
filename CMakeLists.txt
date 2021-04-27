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
elseif(BUILD_TARGET STREQUAL "BUILD_DEB_18_04")
    find_package(PROJ4 REQUIRED)
    set(APP_DATA_PATH /usr/share/pprzgcs)
elseif(BUILD_TARGET STREQUAL "DEV_20_04")
    get_filename_component(APP_DATA_PATH "./data/" ABSOLUTE)
    set(PROJ4_LIBRARIES proj)
elseif(BUILD_TARGET STREQUAL "BUILD_DEB_20_04")
    set(APP_DATA_PATH /usr/share/pprzgcs)
    set(PROJ4_LIBRARIES proj)
elseif(BUILD_TARGET STREQUAL "BUILD_MAC")
    message(FATAL_ERROR "Building for MAC not yet supported!")
else()
    message(FATAL_ERROR "You must define BUILD_TARGET to one of: DEV_18_04, DEV_20_04, BUILD_DEB_20_04, BUILD_DEB_18_04, BUILD_MAC")
endif()



add_definitions(-DAPP_DATA_PATH="${APP_DATA_PATH}")

find_package(Boost COMPONENTS system)

find_package(Qt5 5.12 COMPONENTS Widgets Xml Network Svg REQUIRED)

find_package(tinyxml2 REQUIRED)

find_package(pprzlinkQt REQUIRED)

set(SOURCE
    ${SOURCE}
    resources/ressources.qrc
)

add_subdirectory(src)

include_directories(
    ${INC_DIRS}
)

include_directories(${CMAKE_PREFIX_PATH}/include)
link_directories(${CMAKE_PREFIX_PATH}/lib)

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    Qt5::Svg
    ${PROJ4_LIBRARIES}
    ${Boost_LIBRARIES}
    zip
    pprzlinkQt
    tinyxml2
)


