set(library_SRCS
    ofonointerface.cpp
    ofonomodem.cpp
    ofonomodemmanager.cpp
    )


add_library(ofono-qt SHARED ${library_SRCS})

# Set the library version and the SOVERSION
#set_target_properties(historyservice PROPERTIES
#                      SOVERSION ${HISTORY_VERSION_MAJOR}
#                      VERSION ${HISTORY_VERSION_MAJOR}.${HISTORY_VERSION_MINOR}.${HISTORY_VERSION_PATCH})

qt5_use_modules(ofono-qt Core DBus)
