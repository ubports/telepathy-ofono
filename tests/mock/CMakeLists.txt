set(library_SRCS
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


add_library(ofono-qt SHARED ${library_SRCS})
#set_target_properties(ofono-qt PROPERTIES VERSION  SOVERSION 1.5.0)

# Set the library version and the SOVERSION
#set_target_properties(historyservice PROPERTIES
#                      SOVERSION ${HISTORY_VERSION_MAJOR}
#                      VERSION ${HISTORY_VERSION_MAJOR}.${HISTORY_VERSION_MINOR}.${HISTORY_VERSION_PATCH})

qt5_use_modules(ofono-qt Core DBus)
