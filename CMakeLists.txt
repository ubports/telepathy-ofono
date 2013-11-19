project(telepathy-ofono)
cmake_minimum_required(VERSION 2.8)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Standard install paths
include(GNUInstallDirs)

# Check for include files
include(CheckIncludeFileCXX)

find_package(Qt5Core)
find_package(Qt5DBus)
add_definitions(-DQT_NO_KEYWORDS)

check_include_file_cxx("waudio.h" USE_AUDIOFLINGER)
if (USE_AUDIOFLINGER)
    add_definitions(-DUSE_AUDIOFLINGER)
    set(WAUDIO_LIBRARIES -lwaudio)
endif (USE_AUDIOFLINGER)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TP_QT5 REQUIRED TelepathyQt5)

pkg_check_modules(PULSEAUDIO libpulse)
if (PULSEAUDIO_FOUND)
    add_definitions(-DUSE_PULSEAUDIO)
    set(USE_PULSEAUDIO ON)
    include_directories(${PULSEAUDIO_INCLUDE_DIRS})
endif (PULSEAUDIO_FOUND)

find_program(DBUS_RUNNER dbus-test-runner)

if(NOT DAEMON_DIR)
  set(DAEMON_DIR lib/telepathy)
endif(NOT DAEMON_DIR)

set(CMAKE_AUTOMOC ON)

include_directories(${TP_QT5_INCLUDE_DIRS})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5DBus_INCLUDE_DIRS})
include_directories(/usr/include/telepathy-qt5/)
include_directories(/usr/include/ofono-qt/)

find_library(TELEPATHY_QT5_LIBRARIES telepathy-qt5)
find_library(TELEPATHY_QT5_SERVICE_LIBRARIES telepathy-qt5-service)
find_library(OFONO_QT_LIBRARIES ofono-qt)

if(USE_PULSEAUDIO)
    add_executable(telepathy-ofono main.cpp protocol.cpp connection.cpp ofonotextchannel.cpp ofonocallchannel.cpp voicemailiface.cpp speakeriface.cpp mmsdmanager.cpp mmsdservice.cpp mmsdmessage.cpp qpulseaudioengine.cpp phoneutils.cpp)
else(USE_PULSEAUDIO)
    add_executable(telepathy-ofono main.cpp protocol.cpp connection.cpp ofonotextchannel.cpp ofonocallchannel.cpp voicemailiface.cpp speakeriface.cpp mmsdmanager.cpp mmsdservice.cpp mmsdmessage.cpp phoneutils.cpp)
endif(USE_PULSEAUDIO)
qt5_use_modules(telepathy-ofono Core DBus)

enable_testing()

target_link_libraries(telepathy-ofono ${Qt5Core_LIBRARIES} ${Qt5DBus_LIBRARIES} ${WAUDIO_LIBRARIES} -ltelepathy-qt5 ${TELEPATHY_QT5_SERVICE_LIBRARIES} ${OFONO_QT_LIBRARIES} ${PULSEAUDIO_LIBRARIES})
install(TARGETS telepathy-ofono DESTINATION ${DAEMON_DIR})

configure_file(ofono.service.in org.freedesktop.Telepathy.ConnectionManager.ofono.service)
install (FILES ofono.manager DESTINATION share/telepathy/managers)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.ofono.service DESTINATION share/dbus-1/services)

add_subdirectory(tests)
