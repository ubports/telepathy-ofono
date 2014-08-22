set(library_SRCS
    callvolumeprivate.cpp
    messagemanagerprivate.cpp
    messageprivate.cpp
    modemprivate.cpp
    voicecallmanagerprivate.cpp
    voicecallprivate.cpp
    simmanagerprivate.cpp
    networkregistrationprivate.cpp
    ofonocallvolume.cpp
    ofonointerface.cpp
    ofonomessage.cpp
    ofonomessagemanager.cpp
    ofonomessagewaiting.cpp
    ofonomodem.cpp
    ofonomodeminterface.cpp
    ofonomodemmanager.cpp
    ofononetworkregistration.cpp
    ofonovoicecall.cpp
    ofonovoicecallmanager.cpp
    ofonosimmanager.cpp
    )

include_directories(
    ${CMAKE_BINARY_DIR}
)

qt5_add_dbus_adaptor(library_SRCS CallVolumePrivate.xml callvolumeprivate.h CallVolumePrivate)
qt5_add_dbus_adaptor(library_SRCS ModemPrivate.xml modemprivate.h ModemPrivate)
qt5_add_dbus_adaptor(library_SRCS NetworkRegistrationPrivate.xml networkregistrationprivate.h NetworkRegistrationPrivate)
qt5_add_dbus_adaptor(library_SRCS MessageManagerPrivate.xml messagemanagerprivate.h MessageManagerPrivate)
qt5_add_dbus_adaptor(library_SRCS MessagePrivate.xml messageprivate.h MessagePrivate)
qt5_add_dbus_adaptor(library_SRCS VoiceCallManagerPrivate.xml voicecallmanagerprivate.h VoiceCallManagerPrivate)
qt5_add_dbus_adaptor(library_SRCS VoiceCallPrivate.xml voicecallprivate.h VoiceCallPrivate)
qt5_add_dbus_adaptor(library_SRCS SimManagerPrivate.xml simmanagerprivate.h SimManagerPrivate)

add_library(ofono-qt SHARED ${library_SRCS})

# Set the library version and the SOVERSION
#set_target_properties(historyservice PROPERTIES
#                      SOVERSION ${HISTORY_VERSION_MAJOR}
#                      VERSION ${HISTORY_VERSION_MAJOR}.${HISTORY_VERSION_MINOR}.${HISTORY_VERSION_PATCH})

qt5_use_modules(ofono-qt Core DBus)
