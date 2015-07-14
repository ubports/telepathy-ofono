set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

include(GNUInstallDirs)
include(LibFindMacros)

# Include dir
find_path(LibPhoneNumber_INCLUDE_DIR
  NAMES phonenumberutil.h
  PATHS "/usr/local/${CMAKE_INSTALL_INCLUDEDIR}" ${CMAKE_INSTALL_FULL_INCLUDEDIR}
  PATH_SUFFIXES "phonenumbers"
)

# library itself
find_library(LibPhoneNumber_LIBRARY
  NAMES phonenumber
  PATHS "/usr/local/${CMAKE_INSTALL_LIBDIR}" ${CMAKE_INSTALL_FULL_LIBDIR}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LibPhoneNumber_PROCESS_INCLUDES LibPhoneNumber_INCLUDE_DIR)
set(LibPhoneNumber_PROCESS_LIBS LibPhoneNumber_LIBRARY)
libfind_process(LibPhoneNumber)
