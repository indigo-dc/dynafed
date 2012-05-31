cmake_minimum_required (VERSION 2.8)

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

set(Ugr_SOURCES Config.cc LocationInfo.cc LocationInfoHandler.cc LocationPlugin.cc UgrConnector.cc /
  PluginLoader.cc SimpleDebug.cc)



# Add the binary tree to the search path for include files
# so that we can find our Config header file
include_directories(${PROJECT_BINARY_DIR})

# Set up the includes to be used for boost 
include_directories(${Boost_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS "-O0 -g -Wall")


add_subdirectory(plugins)
add_subdirectory(test)





add_library(UgrConnector SHARED ${Ugr_SOURCES})
target_link_libraries(UgrConnector "dl" ${Boost_LIBRARIES})

# Install directive. This is the library core, to which LFC/DPM should have dependencies
install(TARGETS UgrConnector
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  LIBRARY DESTINATION ${LIBDESTINATION}
  COMPONENT Core)



# Install the include files
install(FILES UgrConnector.hh
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION include
  COMPONENT Core)

install(FILES ${PROJECT_BINARY_DIR}/src/UgrConfig.h
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION include
  COMPONENT Core)

# ----------------------------






