# This module detects if dmlite is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
#
# DMLITE_LIBRARIES = full path to the lfc libraries
# DMLITE_INCLUDE = include dir to be used when using the lfc library
# DMLITE_FOUND   = set to true if lfc was found successfully
#

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  set(LIBDESTINATION "lib64" CACHE STRING "The path suffix where to install libraries. Set this to lib64 if you have such a weird platform that you have to install the libs here.")
ELSE()
  set(LIBDESTINATION "lib" CACHE STRING "The path suffix where to install libraries. Set this to lib64 if you have such a weird platform that you have to install the libs here.")
ENDIF()

message(STATUS "Looking for dmlite.")
# Look for the libraries
find_library(DMLITE_LIBRARY
    NAMES libdmlite.so
    HINTS "/usr/${LIBDESTINATION}"
)
message(STATUS "Found dmlite: ${DMLITE_LIBRARY}")

find_library(DMLITEUTILS_LIBRARY
    NAMES libdmliteutils.so
    HINTS "/usr/${LIBDESTINATION}"
)
message(STATUS "Found dmliteutils: ${DMLITEUTILS_LIBRARY}")

set(DMLITE_LIBRARIES "${DMLITE_LIBRARY} ${DMLITEUTILS_LIBRARY}")

# Look for the include dir
find_path(DMLITE_INCLUDE NAMES dmlite/dmlite.h)

# Set the variables documented above if library was found, fail if not
if(DMLITE_LIBRARY AND DMLITEUTILS_LIBRARY AND DMLITE_INCLUDE)
    if(NOT DMLITE_FIND_QUIETLY)
        message(STATUS "Found dmlite: ${DMLITE_LIBRARIES}")
        message(STATUS "Found dmlite include: ${DMLITE_INCLUDE}")
    endif(NOT DMLITE_FIND_QUIETLY)
    set(DMLITE_FOUND TRUE)
else(DMLITE_LIBRARY AND DMLITEUTILS_LIBRARY AND DMLITE_INCLUDE)
    if(DMLITE_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find dmlite")
    endif(DMLITE_FIND_REQUIRED)
endif(DMLITE_LIBRARY AND DMLITEUTILS_LIBRARY AND DMLITE_INCLUDE)

