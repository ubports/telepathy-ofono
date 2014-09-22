project(telepathy-ofono)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
set(TELEPATHY_OFONO telepathy-ofono)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Standard install paths
include(GNUInstallDirs)

# Check for include files
include(CheckIncludeFileCXX)

include(EnableCoverageReport)
#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -coverage" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -coverage" )
  ENABLE_COVERAGE_REPORT(TARGETS ${TELEPATHY_OFONO})
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

find_package(Qt5Core)
find_package(Qt5DBus)
add_definitions(-DQT_NO_KEYWORDS)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TP_QT5 REQUIRED TelepathyQt5)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

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
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(/usr/include/telepathy-qt5/)
include_directories(/usr/include/ofono-qt/)

find_library(TELEPATHY_QT5_LIBRARIES telepathy-qt5)
find_library(TELEPATHY_QT5_SERVICE_LIBRARIES telepathy-qt5-service)
find_library(OFONO_QT_LIBRARIES ofono-qt)

qt5_add_resources(telepathyfono_RES sqlitetelepathyofono.qrc)

# update the .qrc file automatically when there are new schema files
file(GLOB QRC_RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/*.sql ${CMAKE_CURRENT_SOURCE_DIR}/schema/*.info)
set(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sqlitetelepathyofono.qrc)
add_custom_command(
    OUTPUT ${QRC_FILE}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update_qrc.sh ${QRC_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${QRC_RESOURCE_FILES}
    )
add_custom_target(qrc_update DEPENDS ${QRC_FILE} schema_update)

set(TELEPATHY_OFONO_SRC 
   main.cpp
   protocol.cpp
   connection.cpp
   ofonotextchannel.cpp
   ofonocallchannel.cpp
   ofonoconferencecallchannel.cpp
   emergencymodeiface.cpp
   voicemailiface.cpp
   audiooutputsiface.cpp
   mmsdmanager.cpp
   mmsdservice.cpp
   mmsdmessage.cpp
   pendingmessagesmanager.cpp
   phoneutils.cpp
   powerdaudiomodemediator.cpp
   powerddbus.cpp
   sqlitedatabase.cpp
   ussdiface.cpp
   ${telepathyfono_RES})

if(USE_PULSEAUDIO)
    add_executable(${TELEPATHY_OFONO} qpulseaudioengine.cpp ${TELEPATHY_OFONO_SRC})
else(USE_PULSEAUDIO)
    add_executable(${TELEPATHY_OFONO} ${TELEPATHY_OFONO_SRC})
endif(USE_PULSEAUDIO)
qt5_use_modules(${TELEPATHY_OFONO} Core DBus Sql)
add_dependencies(${TELEPATHY_OFONO} schema_update qrc_update)

enable_testing()

target_link_libraries(${TELEPATHY_OFONO} ${Qt5Core_LIBRARIES} ${Qt5DBus_LIBRARIES} ${WAUDIO_LIBRARIES} -ltelepathy-qt5 ${TELEPATHY_QT5_SERVICE_LIBRARIES} ${OFONO_QT_LIBRARIES} ${PULSEAUDIO_LIBRARIES} ${SQLITE3_LIBRARIES})
install(TARGETS ${TELEPATHY_OFONO} DESTINATION ${DAEMON_DIR})

configure_file(ofono.service.in org.freedesktop.Telepathy.ConnectionManager.ofono.service)
install (FILES ofono.manager DESTINATION share/telepathy/managers)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.ofono.service DESTINATION share/dbus-1/services)

add_subdirectory(schema)
add_subdirectory(tests)
