cmake_minimum_required (VERSION 2.6)
# Build file for lfc plugin based on GFAL 2.0
# Devresse Adrien

IF(LFC_PLUGIN)

# The source modules
FILE(GLOB src_lfc_plugin "*.cc")

message(STATUS "Building the LFC plugin")

pkg_check_modules(GFAL2_PKG REQUIRED gfal2>=2.1.7)

include_directories(${GFAL2_PKG_INCLUDE_DIRS} )
# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_lfc SHARED ${src_lfc_plugin})
set_target_properties(ugrlocplugin_lfc PROPERTIES LINK_FLAGS "-rdynamic"
                                       LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)

target_link_libraries(ugrlocplugin_lfc ugrconnector ${GFAL2_PKG_LIBRARIES} )


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_lfc
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
  COMPONENT plugins-lfc)

ENDIF(LFC_PLUGIN)

