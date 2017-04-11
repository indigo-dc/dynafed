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


include(CheckIncludeFileCXX)

# include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Configure a header file to pass some of the CMake settings to the source code
# This is useful in order to handle the various system-related defines.
set(CMAKE_REQUIRED_FLAGS_OLD "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++0x")
CHECK_INCLUDE_FILE_CXX(atomic HAVE_ATOMIC)
CHECK_INCLUDE_FILE_CXX(cstdatomic HAVE_CSTDATOMIC)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_OLD}")

configure_file (
	       "${PROJECT_SOURCE_DIR}/src/Config.h.in"
	       "${PROJECT_BINARY_DIR}/src/Config.h"
)

set(Ugr_default_plugin_dir ${PLUGIN_INSTALL_DIR})

add_definitions( -DUGR_PLUGIN_DIR_DEFAULT="${Ugr_default_plugin_dir}")

# Add the binary tree to the search path for include files
# so that we can find our Config header file
message("DIRS: ${PROJECT_BINARY_DIR}/src")

include_directories(BEFORE "${PROJECT_BINARY_DIR}")
include_directories(BEFORE "${PROJECT_BINARY_DIR}/src/")

# Set up the includes to be used for boost 
include_directories("${Boost_INCLUDE_DIRS}")

# Set up the includes to be used for protobuf
include_directories("$PROTOBUF_INCLUDE_DIRS}")

include_directories("." "utils")

# test here, welcome 
# -------------------------
# Local targets
# -------------------------

ADD_CUSTOM_COMMAND(
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UgrMemcached.pb.h ${CMAKE_CURRENT_BINARY_DIR}/UgrMemcached.pb.cc
         COMMAND protoc -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/UgrMemcached.proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         DEPENDS UgrMemcached.proto
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(Ugr_SOURCES UgrConfig.cc LocationInfo.cc LocationInfoHandler.cc LocationPlugin.cc UgrConnector.cc /
  PluginLoader.cc SimpleDebug.cc ExtCacheHandler.cc UgrMemcached.pb.cc PluginInterface.cc UgrLogger.cpp UgrPluginLoader.cc UgrAuthorization.cc)




add_subdirectory(utils)
add_subdirectory(plugins)
add_subdirectory(test)


add_library(ugrconnector SHARED ${Ugr_SOURCES})
set_target_properties(ugrconnector PROPERTIES
 VERSION "${UGR_VERSION_MAJOR}.${UGR_VERSION_MINOR}.${UGR_VERSION_PATCH}"
 SOVERSION "${UGR_VERSION_MAJOR}.${UGR_VERSION_MINOR}.${UGR_VERSION_PATCH}")

target_link_libraries(ugrconnector "dl" ${Boost_LIBRARIES} ${LIBMEMCACHED_LIBRARIES} ${PROTOBUF_LIBRARIES})

# Install directive. This is the library core, to which LFC/DPM should have dependencies
install(TARGETS ugrconnector
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  COMPONENT core)





# auto-generate pkgconfig
add_PkgConfigFile_for_Library("ugr.pc" ugrconnector
                               HEADER_DIRS "ugr"
                               DESCRIPTION "Ugr, Dynamic federation system"
                               REQUIRES "")
                               
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ugr.pc
        DESTINATION ${PKGCONFIG_FILES_DIR} )

# Install the include files
install(FILES UgrConnector.hh
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${INCLUDE_INSTALL_DIR}/ugr/
  COMPONENT core)

install(FILES ${PROJECT_BINARY_DIR}/src/Config.h
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${INCLUDE_INSTALL_DIR}/ugr/
  COMPONENT core)

# ----------------------------






