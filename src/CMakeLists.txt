cmake_minimum_required (VERSION 2.6)

# include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Configure a header file to pass some of the CMake settings to the source code
# This is useful in order to handle the various system-related defines.
configure_file (
	       "${PROJECT_SOURCE_DIR}/src/UgrConfig.h.in"
	       "${PROJECT_BINARY_DIR}/src/UgrConfig.h"
)


# test here, welcome 
# -------------------------
# Local targets
# -------------------------

set(Ugr_SOURCES Config.cc LocationInfo.cc LocationInfoHandler.cc LocationPlugin.cc GeoPlugin.cc UgrConnector.cc /
  PluginLoader.cc SimpleDebug.cc)



# Add the binary tree to the search path for include files
# so that we can find our Config header file
include_directories(${PROJECT_BINARY_DIR})

# Set up the includes to be used for boost 
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(plugins)
add_subdirectory(test)





add_library(ugrconnector SHARED ${Ugr_SOURCES})
target_link_libraries(ugrconnector "dl" ${Boost_LIBRARIES})

# Install directive. This is the library core, to which LFC/DPM should have dependencies
install(TARGETS ugrconnector
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
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

install(FILES ${PROJECT_BINARY_DIR}/src/UgrConfig.h
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${INCLUDE_INSTALL_DIR}/ugr/
  COMPONENT core)

# ----------------------------






