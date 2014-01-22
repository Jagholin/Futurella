cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
project(Futurella)

macro(get_WIN32_WINNT version)
    if (WIN32 AND CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REPLACE "." "" ver ${ver})
        string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

        set(${version} "0x${ver}")
    endif()
endmacro()

if (WIN32)
    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
endif()

find_package(OpenThreads REQUIRED)
find_package(osg REQUIRED)
find_package(osgDB REQUIRED)
find_package(osgUtil REQUIRED)
find_package(osgViewer REQUIRED)
find_package(osgGA REQUIRED)
find_package(CEGUI REQUIRED)
find_package(OpenGL REQUIRED)

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
    src/networking/peermanager.h
    src/networking/peermanager.cpp
    src/networking/sigslot.h
    src/gamecommon/GameMessagePeer.h
    src/gamecommon/GameMessagePeer.cpp
    src/gamecommon/GameObject.h
    src/gamecommon/GameObject.cpp
    src/gameserver/SpaceShipServer.h
    src/gameclient/SpaceShipClient.h
    src/CEGUIDrawable.h
    src/CEGUIDrawable.cpp
    src/GUIApplication.h
    src/GUIApplication.cpp
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
add_lib(Futurella OSGDB)
add_lib(Futurella OSGUTIL)
add_lib(Futurella OSGVIEWER)
add_lib(Futurella OSGGA)
add_lib(Futurella CEGUI)
add_lib(Futurella Boost)
add_lib(Futurella OPENGL)
