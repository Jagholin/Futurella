cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

project(Futurella)

find_package(OpenThreads REQUIRED)
find_package(osg REQUIRED)
find_package(osgDB REQUIRED)
find_package(osgUtil REQUIRED)
find_package(osgViewer REQUIRED)
find_package(CEGUI REQUIRED)

find_package(BOOST 1.54.0 REQUIRED COMPONENTS system asio)

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

link_directories(${Boost_LIBRARY_DIRS}) # for Boost dynamic linking

add_executable(Futurella
	src/networking/networking.cpp
	src/networking/networking.h
	src/networking/messages.cpp
	src/networking/messages.h
	src/CEGUIDrawable.h
	src/CEGUIDrawable.cpp
    src/main.cpp)

add_lib(Futurella OPENTHREADS)
add_lib(Futurella OSG)
add_lib(Futurella OSGDB)
add_lib(Futurella OSGUTIL)
add_lib(Futurella OSGVIEWER)
add_lib(Futurella CEGUI)
add_lib(Futurella Boost)