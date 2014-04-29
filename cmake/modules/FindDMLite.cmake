# This module detects if dmlite is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
#
# DMLITE_LIBRARIES = full path to the lfc libraries
# DMLITE_INCLUDE = include dir to be used when using the lfc library
# DMLITE_FOUND   = set to true if lfc was found successfully
#

if(NOT LIBDESTINATION)
  set(LIBDESTINATION "lib")
endif()

message(STATUS "Looking for dmlite... /usr/${LIBDESTINATION}/")
# Look for the libraries
find_library(DMLITE_LIBRARY
    NAMES dmlite
    HINTS "/usr/${LIBDESTINATION}/"
    HINTS "/usr/lib64/"
    HINTS "/usr/local/lib64/"
)
message(STATUS "Found dmlite: ${DMLITE_LIBRARY}")


set(DMLITE_LIBRARIES "${DMLITE_LIBRARY}")

# Look for the include dir
find_path(DMLITE_INCLUDE NAMES dmlite/cpp/dmlite.h 
PATHS /usr/include /usr/local/include 
)
message(STATUS "Found dmlite include: ${DMLITE_INCLUDE}")

# Set the variables documented above if library was found, fail if not
if(DMLITE_LIBRARY AND DMLITE_INCLUDE)
    if(NOT DMLITE_FIND_QUIETLY)
        message(STATUS "Found dmlite: ${DMLITE_LIBRARIES} - includes: ${DMLITE_INCLUDE}")
    endif(NOT DMLITE_FIND_QUIETLY)
    set(DMLITE_FOUND TRUE)
else(DMLITE_LIBRARY AND DMLITEUTILS_LIBRARY AND DMLITE_INCLUDE)
    if(DMLITE_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find dmlite")
    endif(DMLITE_FIND_REQUIRED)
endif(DMLITE_LIBRARY AND DMLITE_INCLUDE)

