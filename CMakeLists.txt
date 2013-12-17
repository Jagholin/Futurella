cmake_minimum_required(VERSION 2.6)

project(Futurella)

find_package(OpenThreads REQUIRED)
find_package(osgGA REQUIRED)
find_package(osg REQUIRED)
find_package(osgDB REQUIRED)
find_package(osgUtil REQUIRED)
find_package(osgViewer REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY          ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG    ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY          ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG    ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  ${CMAKE_SOURCE_DIR}/bin)

macro(add_lib PROJNAME LIBNAME)
    include_directories( ${${LIBNAME}_INCLUDE_DIR} )
    target_link_libraries( ${PROJNAME} ${${LIBNAME}_LIBRARIES} )
endmacro()

add_executable(Futurella 
src/main.cpp
src/Asteroid.cpp
src/Asteroid.h
src/AsteroidField.cpp
src/AsteroidField.h
src/Level.cpp
src/Level.h
src/Node.cpp
src/Node.h
src/ChaseCam.cpp
src/ChaseCam.h
src/SpaceShip.cpp
src/SpaceShip.h)

add_lib(Futurella OPENTHREADS)
add_lib(Futurella OSG)
add_lib(Futurella OSGGA)
add_lib(Futurella OSGDB)
add_lib(Futurella OSGUTIL)
add_lib(Futurella OSGVIEWER)