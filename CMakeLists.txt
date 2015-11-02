cmake_minimum_required (VERSION 2.6)

#
#  Copyright (c) CERN 2013
#
#  Copyright (c) Members of the EMI Collaboration. 2011-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
#
#  Licensed under the Apache License, Version 2.0
#  See the LICENSE file for further information
#



project (ugr)

# build type
set(CMAKE_BUILD_TYPE "RelWithDebInfo"
    CACHE STRING "build type")

## plugin options
OPTION(LFC_PLUGIN "Enable LFC client side plugin" TRUE)
OPTION(GEOIP_PLUGIN "Enable GeoIP plugin for geography based redirection" TRUE)
OPTION(DAV_PLUGIN "Enable Webdav client side plugin" TRUE)
OPTION(S3_PLUGIN "Enable S3 client side plugin" TRUE)
OPTION(AZURE_PLUGIN "Enable MS Azure client side plugin" TRUE)
OPTION(HTTP_PLUGIN "Enable HTTP client side plugin" TRUE)


## support option
OPTION(RSYSLOG_SUPPORT "Enable rsyslog Support inside UGR" FALSE)
OPTION(LOGROTATE_SUPPORT "Enable logrotate Support for UGR" FALSE)

# tests options
option(UNIT_TESTS "enable or disable the unit tests" FALSE)

# The version number
set (UGR_VERSION_MAJOR 1)
set (UGR_VERSION_MINOR 0)
set (UGR_VERSION_PATCH 18)


# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (eg MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# import common cmake macros
include(DefineInstallationPaths REQUIRED)
include(DefineApachePaths REQUIRED)
include(ReleaseDebugAutoFlags REQUIRED)
include(CMakeGeneratePkgConfig REQUIRED)
include(CMakeUninstall REQUIRED)
include(MacroOutOfSourceBuild)
#enable testing
INCLUDE(CTest)
 
macro_ensure_out_of_source_build("$(PROJECT_NAME) requires an out of source build.")

# enable 64 bits off_t on 32 bits plateforms
add_definitions( -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE )

# enable C++11
add_definitions( -std=c++0x)



# Look for the external packages that are needed, for both the core and the plugins
set (Boost_NO_BOOST_CMAKE ON) # fix boost problem on EL6
find_package( Boost 1.41.0 COMPONENTS date_time filesystem system thread REQUIRED)

# Look for the LFC client
#find_package(LFC 1.8.0)

# Look for libDMlite
find_package(DMLite 0.3)

# generic pkgconfig module for Davix & co
find_package(PkgConfig REQUIRED)

#MaxMind GeoIP
find_package(GeoIP)

#Memcached
find_package(LibMemCached REQUIRED)

#Protobuf
find_package(Protobuf REQUIRED)

pkg_check_modules(GFAL2_PKG gfal2>=2.1.7)

add_subdirectory(packaging)
add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(dist)
add_subdirectory(test)

enable_testing()

