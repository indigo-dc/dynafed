## Tries to find the required MaxMind GeoIPlibraries. Once done this will
## define the variable GEOIP_LIBRARIES.

message(STATUS "Searching for MaxMindDB")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMMDB_PC maxminddb)

IF(LIBMMDB_PC_FOUND)
    message(STATUS "MaxMindDB pkgconfig file - found")
    SET(MMDB_GEO_LIB 1)
    SET(MMDB_GEO_INCLUDE_DIRS ${LIBMMDB_PC_INCLUDE_DIRS})
    SET(MMDB_GEO_LIB ${LIBMMDB_PC_LIBRARIES})
    SET(MMDB_GEO_LIBRARIES ${LIBMMDB_PC_LIBRARIES})

    set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH} ${LIBMMDB_INCLUDE_DIRS}")
ELSE(LIBMMDB_PC_FOUND)

  set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH} ${LIBMMDB_INCLUDE_DIRS}")
  find_path(HAVE_MMDB_H NAMES "maxminddb.h" HINTS ${LIBMMDB_INCLUDE_DIR} )

  if (HAVE_MMDB_H)
    message(STATUS "Looking for MaxMind DB header files - found in ${HAVE_MMDB_H}")
  else(HAVE_MMDB_H)
    message(FATAL_ERROR "Could not find one or more MaxMind DB header files. If the MaxMind DB library is installed, you can run CMake again and specify its location with -DGMMDB_INCLUDE_DIR=<path>")
  endif(HAVE_MMDB_H)

  message(STATUS "Looking for MaxMind DB libraries")
  find_library(MMDB_GEO_LIB
    NAMES maxminddb
    PATHS ${MMDB_LIBRARY_DIR}
  )


  set(MMDB_GEO_LIBRARIES ${MMDB_GEO_LIB})

ENDIF(LIBMMDB_PC_FOUND)


  IF(MMDB_GEO_LIB)
    message(STATUS "Looking for MaxMind DB library - found ${MMDB_GEO_LIB}")
  else(MMDB_GEO_LIB)
    message(FATAL_ERROR "Could not find MaxMind DB library. If the MaxMind DB library is installed, you can run CMake again and specify its location with -DMMDB_LIBRARY_DIR=<path>")
  ENDIF(MMDB_GEO_LIB)