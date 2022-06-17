cmake_minimum_required(VERSION 3.10)

project(pprzgcs)

include(GNUInstallDirs)

set(CXX_EXTRA_FLAGS "-Wall -Wextra -fno-sized-deallocation")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_EXTRA_FLAGS} -Werror")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

find_package(IvyQt REQUIRED)
find_package(pprzlinkQt REQUIRED)

set(Qt5_DIR "/home/fabien/Qt/5.15.2/gcc_64/lib/cmake/Qt5")

if(SPEECH)
    message("SPEECH ENABLED !")
    add_definitions(-DSPEECH_ENABLED)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets Xml Network Svg TextToSpeech REQUIRED)
else()
    message("SPEECH DISABLED !")
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets Xml Network Svg  REQUIRED)
endif()




add_subdirectory(src)

include_directories(
    ${INC_DIRS}
    ${LIBXML2_INCLUDE_DIR}
)


# Enable policy to run automoc on generated files.
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

find_package(PythonInterp 3.8 REQUIRED)
find_program(VIRTUALENV virtualenv)
if(NOT VIRTUALENV)
    message(FATAL_ERROR "Could not find `virtualenv` in PATH")
endif()

set (ENV{VIRTUAL_ENV} "/home/fabien/DEV/testenv")

SET(python_interpreter /home/fabien/DEV/testenv/bin/python)

# Use provided python interpreter if given.
if(NOT python_interpreter)
    message("no python interpreter provided, searching...")
    find_program(python_interpreter "python")
endif()
message(STATUS "Using python interpreter: ${python_interpreter}")

# Macro to get various pyside / python include / link flags.
macro(pyside2_config option output_var)
    if(${ARGC} GREATER 2)
        set(is_list ${ARGV2})
    else()
        set(is_list "")
    endif()

    execute_process(
      COMMAND ${python_interpreter} "${CMAKE_SOURCE_DIR}/python/utils/pyside2_config.py"
              ${option}
      OUTPUT_VARIABLE ${output_var}
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if ("${${output_var}}" STREQUAL "")
        message(FATAL_ERROR "Error: Calling pyside2_config.py ${option} returned no output.")
    endif()
    if(is_list)
        string (REPLACE " " ";" ${output_var} "${${output_var}}")
    endif()
endmacro()

# Query for the shiboken2-generator path, PySide2 path, Python path, include paths and linker flags.
pyside2_config(--shiboken2-module-path SHIBOKEN2_MODULE_PATH)
pyside2_config(--shiboken2-generator-path SHIBOKEN2_GENERATOR_PATH)
pyside2_config(--pyside2-path PYSIDE2_PATH)

pyside2_config(--python-include-path PYTHON_INCLUDE_DIR)
pyside2_config(--shiboken2-generator-include-path SHIBOKEN2_GENERATOR_INCLUDE_DIR 1)
pyside2_config(--pyside2-include-path PYSIDE2_INCLUDE_DIR 1)

pyside2_config(--python-link-flags-cmake PYTHON_LINKING_DATA 0)
pyside2_config(--shiboken2-module-shared-libraries-cmake SHIBOKEN2_MODULE_SHARED_LIBRARIES 0)
pyside2_config(--pyside2-shared-libraries-cmake PYSIDE2_SHARED_LIBRARIES 0)

set(SHIBOKEN_PATH "${SHIBOKEN2_GENERATOR_PATH}/shiboken2${CMAKE_EXECUTABLE_SUFFIX}")

if(NOT EXISTS ${SHIBOKEN_PATH})
    message(FATAL_ERROR "Shiboken executable not found at path: ${SHIBOKEN_PATH}")
endif()

# Get all relevant Qt include dirs, to pass them on to shiboken.
get_property(QT_CORE_INCLUDE_DIRS TARGET Qt5::Core PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
get_property(QT_GUI_INCLUDE_DIRS TARGET Qt5::Gui PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
get_property(QT_WIDGETS_INCLUDE_DIRS TARGET Qt5::Widgets PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
get_property(QT_XML_INCLUDE_DIRS TARGET Qt5::Xml PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
set(QT_INCLUDE_DIRS ${QT_CORE_INCLUDE_DIRS} ${QT_GUI_INCLUDE_DIRS} ${QT_WIDGETS_INCLUDE_DIRS} ${QT_XML_INCLUDE_DIRS})
set(INCLUDES "")
foreach(INCLUDE_DIR ${QT_INCLUDE_DIRS} ${INC_DIRS})
    list(APPEND INCLUDES "-I${INCLUDE_DIR}")
endforeach()

# Set up the options to pass to shiboken.
set(WRAPPED_HEADER ${CMAKE_SOURCE_DIR}/src/wrappedclasses.h)
set(TYPESYSTEM_FILE ${CMAKE_SOURCE_DIR}/src/pprzgcs.xml)

set(SHIBOKEN_OPTIONS --generator-set=shiboken --enable-parent-ctor-heuristic
    --enable-pyside-extensions --enable-return-value-heuristic --use-isnull-as-nb_nonzero
    --avoid-protected-hack
    ${INCLUDES}
    -I${CMAKE_SOURCE_DIR}
    -T${CMAKE_SOURCE_DIR}
    -T${PYSIDE2_PATH}/typesystems
    --output-directory=${CMAKE_CURRENT_BINARY_DIR}
    )

# Specify which sources will be generated by shiboken, and their dependencies.
set(GENERATED_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/AppLib/applib_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AppLib/pprzmain_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AppLib/waypoint_wrapper.cpp
    )

set(GENERATED_SOURCES_DEPENDENCIES
    ${WRAPPED_HEADER}
    ${TYPESYSTEM_FILE}
    )

# Add custom target to run shiboken.
add_custom_command(OUTPUT ${GENERATED_SOURCES}
                    COMMAND ${SHIBOKEN_PATH}
                    ${SHIBOKEN_OPTIONS} ${WRAPPED_HEADER} ${TYPESYSTEM_FILE}
                    DEPENDS ${GENERATED_SOURCES_DEPENDENCIES}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Running generator for ${TYPESYSTEM_FILE}.")



# We need to include the headers for the module bindings that we use.
set(PYSIDE2_ADDITIONAL_INCLUDES "")
foreach(INCLUDE_DIR ${PYSIDE2_INCLUDE_DIR})
    list(APPEND PYSIDE2_ADDITIONAL_INCLUDES "${INCLUDE_DIR}/QtCore")
    list(APPEND PYSIDE2_ADDITIONAL_INCLUDES "${INCLUDE_DIR}/QtGui")
    list(APPEND PYSIDE2_ADDITIONAL_INCLUDES "${INCLUDE_DIR}/QtWidgets")
endforeach()

# =============================================================================================
# !!! (The section below is deployment related, so in a real world application you will want to
# take care of this properly with some custom script or tool).
# =============================================================================================
# Enable rpaths so that the example can be executed from the build dir.
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${PYSIDE2_PATH} ${SHIBOKEN2_MODULE_PATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# =============================================================================================
# !!! End of dubious section.
# =============================================================================================







set(SOURCE
    ${SOURCE}
    ${GENERATED_SOURCES}
    src/pythonutils.cpp
    resources/ressources.qrc
)






add_executable(${PROJECT_NAME} ${SOURCE})


# Enable automoc.
set_property(TARGET ${PROJECT_NAME} PROPERTY AUTOMOC 1)


# Apply relevant include and link flags.
target_include_directories(${PROJECT_NAME} PRIVATE ${PYTHON_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SHIBOKEN2_GENERATOR_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${PYSIDE2_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${PYSIDE2_ADDITIONAL_INCLUDES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})




target_link_libraries(${PROJECT_NAME} PRIVATE ${SHIBOKEN2_MODULE_SHARED_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PYSIDE2_SHARED_LIBRARIES})






target_link_libraries(${PROJECT_NAME} PRIVATE
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
)




# Find and link to the python library.
list(GET PYTHON_LINKING_DATA 0 PYTHON_LIBDIR)
list(GET PYTHON_LINKING_DATA 1 PYTHON_LIB)
find_library(PYTHON_LINK_FLAGS ${PYTHON_LIB} PATHS ${PYTHON_LIBDIR} HINTS ${PYTHON_LIBDIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PYTHON_LINK_FLAGS})

# Same as CONFIG += no_keywords to avoid syntax errors in object.h due to the usage of the word Slot
#target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)








install(TARGETS ${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/data/ DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")

add_subdirectory(assets)


