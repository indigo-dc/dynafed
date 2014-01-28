cmake_minimum_required (VERSION 2.6)
# Build file for the Http plugin
# Fabrizio Furano
# Taken by the dav one (Adrien Devresse)

IF(HTTP_PLUGIN OR DAV_PLUGIN)

# The source modules
FILE(GLOB src_http_plugin "*http.cc")
SET(src_http_plugin ${src_http_plugin} PARENT_SCOPE)
FILE(GLOB src_http_plugin_entry "*entry.cc")

message(STATUS "Building the HTTP plugin")

pkg_check_modules(DAVIX_PKG REQUIRED davix)
# glibmm for dav plugin
include_directories(${DAVIX_PKG_INCLUDE_DIRS} )
# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_http SHARED ${src_http_plugin} ${src_http_plugin_entry})
set_target_properties(ugrlocplugin_http PROPERTIES LINK_FLAGS "-rdynamic"
                                       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/plugins)

target_link_libraries(ugrlocplugin_http ugrconnector ${DAVIX_PKG_LIBRARIES} rt)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_http
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
  COMPONENT plugins-http)



ENDIF(HTTP_PLUGIN OR DAV_PLUGIN)
