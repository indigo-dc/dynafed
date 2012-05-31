## Tries to find the required MaxMind GeoIPlibraries. Once done this will
## define the variable GEOIP_LIBRARIES.

message(STATUS "Looking for MaxMind GeoIP header files")

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
if (GEOIP_LIB)
  message(STATUS "Looking for MaxMind GeoIP libraries - found")
  set(GEOIP_LIBRARIES ${GEOIP_LIB})
else(GEOIP_LIB)
  message(FATAL_ERROR "Could not find MaxMind GeoIP library")
endif(GEOIP_LIB)