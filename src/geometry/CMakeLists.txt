
set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/coordinatestransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/point2dlatlon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/point2dtile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/point2dpseudomercator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coordinate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cartesian_coor.cpp
)

set(INC_DIRS ${INC_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCE ${SOURCE}  PARENT_SCOPE)
set(INC_DIRS ${INC_DIRS}  PARENT_SCOPE)
