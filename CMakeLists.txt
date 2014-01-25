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

find_package(CEGUI REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLOW REQUIRED)

find_package(BOOST 1.54.0 REQUIRED COMPONENTS system asio)
# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

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
    src/gameserver/SpaceShipServer.cpp
    src/gameserver/GameInstanceServer.h
    src/gameserver/GameInstanceServer.cpp
    src/gameserver/AsteroidFieldServer.h
    src/gameserver/AsteroidFieldServer.cpp
    src/gameclient/SpaceShipClient.h
    src/gameclient/SpaceShipClient.cpp
    src/gameclient/GameInstanceClient.h
    src/gameclient/GameInstanceClient.cpp
    src/gameclient/AsteroidFieldClient.h
    src/gameclient/AsteroidFieldClient.cpp
    src/CEGUIDrawable.h
    src/CEGUIDrawable.cpp
    src/GUIApplication.h
    src/GUIApplication.cpp
    src/main.cpp
    src/Asteroid.cpp
    src/Asteroid.h
    src/AsteroidField.cpp
    src/AsteroidField.h
    src/Node.cpp
    src/Node.h
    src/ChaseCam.cpp
    src/ChaseCam.h
    src/gameclient/LevelDrawable.cpp
    src/gameclient/LevelDrawable.h)

add_lib(Futurella OPENTHREADS)
add_lib(Futurella CEGUI)
add_lib(Futurella Boost)
add_lib(Futurella OPENGL)
add_lib(Futurella GLOW)
add_lib(Futurella GLOW_UTILS)
add_lib(Futurella GLOW_WINDOW)
