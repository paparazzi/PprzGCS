
set(SOURCE ${SOURCE})

if(SPEECH)
    list(APPEND SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/speaker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/srtm_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/coordinatestransform.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dispatcher_ui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/pprz_dispatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AircraftManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/units.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/watcher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/python_plugins.cpp
        )
endif()

set(INC_DIRS ${INC_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCE ${SOURCE}  PARENT_SCOPE)
set(INC_DIRS ${INC_DIRS}  PARENT_SCOPE)
