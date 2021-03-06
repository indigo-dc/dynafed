cmake_minimum_required (VERSION 2.6)
# Build file for s3 plugin
# Devresse Adrien


IF(S3_PLUGIN)


# The source modules
FILE(GLOB src_s3_plugin "*s3.cc")
SET(src_s3_plugin ${src_s3_plugin} PARENT_SCOPE)
FILE(GLOB src_s3_plugin_entry "*entry.cc")

message(STATUS "Building the S3 plugin")

pkg_check_modules(DAVIX_PKG REQUIRED davix>=0.5.1)

include_directories(${DAVIX_PKG_INCLUDE_DIRS} )
# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_s3 SHARED ${src_s3_plugin} ${src_http_plugin} ${src_s3_plugin_entry})
set_target_properties(ugrlocplugin_s3 PROPERTIES LINK_FLAGS "-rdynamic"
                                       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)

target_link_libraries(ugrlocplugin_s3 ugrconnector ${DAVIX_PKG_LIBRARIES} rt)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_s3
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
  COMPONENT plugins-s3)


ENDIF(S3_PLUGIN)
