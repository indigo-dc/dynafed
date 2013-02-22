## Tries to find the required MaxMind GeoIPlibraries. Once done this will
## define the variable GEOIP_LIBRARIES.

message(STATUS "Searching for GEOIP")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGEOIP_PC geoip)

IF(LIBGEOIP_PC_FOUND)
     message(STATUS "GeoIP pkgconfig file - found")
    SET(GEOIP_LIB 1)
    SET(GEOIP_INCLUDE_DIRS ${LIBGEOIP_PC_INCLUDE_DIRS})
    SET(GEOIP_LIB ${LIBGEOIP_PC_LIBRARIES})
    SET(GEOIP_LIBRARIES ${LIBGEOIP_PC_LIBRARIES})

ELSE(LIBGEOIP_PC_FOUND)

set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH} ${GEOIP_INCLUDE_DIR}")

find_path(HAVE_GEOIP_H NAMES "GeoIP.h")
find_path(HAVE_GEOIPCITY_H NAMES "GeoIPCity.h")


if (HAVE_GEOIP_H AND HAVE_GEOIPCITY_H)
  message(STATUS "Looking for MaxMind GeoIP header files - found")
else(HAVE_GEOIP_H AND HAVE_GEOIPCITY_H)
  message(FATAL_ERROR "Could not find one or more MaxMind GeoIP header files. If the MaxMind GeoIP library is installed, you can run CMake again and specify its location with -DGEOIP_INCLUDE_DIR=<path>")
endif(HAVE_GEOIP_H AND HAVE_GEOIPCITY_H)

message(STATUS "Looking for MaxMind GeoIP libraries")
find_library(GEOIP_LIB
  NAMES GeoIP geoip
  PATHS ${GEOIP_LIBRARY_DIR}
)

set(GEOIP_LIBRARIES ${GEOIP_LIB})

ENDIF(LIBGEOIP_PC_FOUND)


IF(NOT GEOIP_LIB)
  message(FATAL_ERROR "Could not find MaxMind GeoIP library")
ENDIF(NOT GEOIP_LIB)

