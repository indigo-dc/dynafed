cmake_minimum_required (VERSION 2.6)
# Build file for Webdav plugin
# Devresse Adrien

IF(PLUGIN_WEBDAV)

# The source modules
FILE(GLOB src_dav_plugin "*.cc")


message(STATUS "Building the DAV plugin")

pkg_check_modules(DAVIX_PKG REQUIRED davix)

include_directories(${DAVIX_PKG_INCLUDE_DIRS})
# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_dav SHARED ${src_dav_plugin})
SET_TARGET_PROPERTIES(ugrlocplugin_dav PROPERTIES LINK_FLAGS "-rdynamic")

target_link_libraries(ugrlocplugin_dav ugrconnector ${DAVIX_PKG_LIBRARIES})


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_dav
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${LIBDESTINATION}
  COMPONENT plugins-webdav)

ENDIF(PLUGIN_WEBDAV)
