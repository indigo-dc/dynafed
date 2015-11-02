cmake_minimum_required (VERSION 2.6)
# Build file for s3 plugin
# Devresse Adrien


IF(AZURE_PLUGIN)


# The source modules
FILE(GLOB src_azure_plugin "*.cc")

message(STATUS "Building the Azure plugin")

pkg_check_modules(DAVIX_PKG REQUIRED davix>=0.5.1)

include_directories(${DAVIX_PKG_INCLUDE_DIRS} )

# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_azure SHARED ${src_azure_plugin})
set_target_properties(ugrlocplugin_azure PROPERTIES LINK_FLAGS "-rdynamic"
                                       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)
                                       
target_link_libraries(ugrlocplugin_azure ugrconnector ${DAVIX_PKG_LIBRARIES} rt)

# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_azure
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
  COMPONENT plugins-azure)


ENDIF(AZURE_PLUGIN)
