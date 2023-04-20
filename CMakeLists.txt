cmake_minimum_required(VERSION 3.10)

project(pprzgcs)

include(GNUInstallDirs)

set(CXX_EXTRA_FLAGS "-Wall -Wextra -fno-sized-deallocation")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_EXTRA_FLAGS} -Werror")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# find library proj (from 20.04 packages) or PROJ4 (from source)
find_library(PROJ proj)
if(${PROJ} STREQUAL "PROJ-NOTFOUND")
    find_package(PROJ4 REQUIRED)
else()
    set(PROJ4_LIBRARIES proj)
endif()

find_package(LibXml2 REQUIRED)

if(NOT DEFAULT_APP_DATA_PATH)
    set(DEFAULT_APP_DATA_PATH "/usr/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
endif()
message("DEFAULT_APP_DATA_PATH set to ${DEFAULT_APP_DATA_PATH}")
add_definitions(-DDEFAULT_APP_DATA_PATH="${DEFAULT_APP_DATA_PATH}")

if(NOT PPRZGCS_VERSION)
    if ("${PPRZGCS_VERSION}" STREQUAL "")
        set(PPRZGCS_VERSION "devel")
    endif()
endif()
message("PPRZGCS_VERSION set to ${PPRZGCS_VERSION}")
add_definitions(-DPPRZGCS_VERSION="${PPRZGCS_VERSION}")

option(SPEECH "Enable speech" ON)
option(GRPC "Enable GRPC" OFF)

find_package(IvyQt REQUIRED)
find_package(pprzlinkQt REQUIRED)

# minumum required Qt components
set(QT_COMPONENTS Widgets Xml Network Svg)

if(SPEECH)
    message("SPEECH ENABLED !")
    add_definitions(-DSPEECH_ENABLED)
    list(APPEND QT_COMPONENTS TextToSpeech)
else()
    message("SPEECH DISABLED !")
endif()


if(DEFINED ENV{QT_DIR})
    set(Qt5_DIR $ENV{QT_DIR})
    set(Qt6_DIR $ENV{QT_DIR})
endif()

message("Qt required components: ${QT_COMPONENTS}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_COMPONENTS})
message("Using Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}: ${Qt5Core_DIR}")

if(${GRPC})
    message("GRPC ENABLED !")
    add_definitions(-DGRPC_ENABLED)
    set(PPRZGCS_GRPC_PREFIX ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
    add_subdirectory(protos)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}/protos")
else()
    message("GRPC DISABLED !")
endif()



set(SOURCE
    ${SOURCE}
    resources/ressources.qrc
)

add_subdirectory(src)

include_directories(
    ${INC_DIRS}
    ${LIBXML2_INCLUDE_DIR}
)



add_executable(${PROJECT_NAME} ${SOURCE})

if(${GRPC})
    add_dependencies(
        ${PROJECT_NAME}
        proto_py
    )
endif()

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    Qt5::Svg
    $<$<BOOL:${SPEECH}>:Qt5::TextToSpeech>
    ${PROJ4_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    zip
    pprzlinkQt
    IvyQt
    $<$<BOOL:${GRPC}>:PprzgcsGRCP>
    $<$<BOOL:${GRPC}>:${_PROTOBUF_LIBPROTOBUF}>
    $<$<BOOL:${GRPC}>:${_GRPC_GRPCPP}>
    $<$<BOOL:${GRPC}>:${_REFLECTION}>
)

install(TARGETS ${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/data/ DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")

add_subdirectory(assets)


