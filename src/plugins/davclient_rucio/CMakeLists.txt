cmake_minimum_required (VERSION 2.6)
# Build file for Webdav plugin
# Devresse Adrien


IF(DAV_PLUGIN)


# The source modules
FILE(GLOB src_davrucio_plugin "*.cc")

message(STATUS "Building the DAV Rucio-aware plugin")

pkg_check_modules(DAVIX_PKG REQUIRED davix>=0.0.20)

include_directories(${DAVIX_PKG_INCLUDE_DIRS} )
# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_davrucio SHARED ${src_http_plugin} ${src_dav_plugin} ${src_davrucio_plugin})
set_target_properties(ugrlocplugin_davrucio PROPERTIES LINK_FLAGS "-rdynamic"
                                       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/plugins)

target_link_libraries(ugrlocplugin_davrucio ugrconnector ${DAVIX_PKG_LIBRARIES} rt)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_davrucio
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
  COMPONENT plugins-webdav)


ENDIF(DAV_PLUGIN)
