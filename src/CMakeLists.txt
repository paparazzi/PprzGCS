
set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PprzToolbox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PprzApplication.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AircraftManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/srtm_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aircraft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/app_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/configurator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pprz_dispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pprzmain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dispatcher_ui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/globalconfig.cpp
)

set(INC_DIRS ${INC_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(widgets)
add_subdirectory(common)
add_subdirectory(geometry)
add_subdirectory(tools)

set(SOURCE ${SOURCE}  PARENT_SCOPE)
set(INC_DIRS ${INC_DIRS}  PARENT_SCOPE)


