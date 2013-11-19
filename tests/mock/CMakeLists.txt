set(library_SRCS
    modemprivate.cpp
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
    )

include_directories(
    ${CMAKE_BINARY_DIR}
)

qt5_add_dbus_adaptor(library_SRCS ModemPrivate.xml modemprivate.h ModemPrivate)

add_library(ofono-qt SHARED ${library_SRCS})

# Set the library version and the SOVERSION
#set_target_properties(historyservice PROPERTIES
#                      SOVERSION ${HISTORY_VERSION_MAJOR}
#                      VERSION ${HISTORY_VERSION_MAJOR}.${HISTORY_VERSION_MINOR}.${HISTORY_VERSION_PATCH})

qt5_use_modules(ofono-qt Core DBus)
