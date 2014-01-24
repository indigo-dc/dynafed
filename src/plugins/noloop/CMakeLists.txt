cmake_minimum_required (VERSION 2.6)

#
# no loop plugin
#

FILE(GLOB src_noloop "*.cc")

pkg_check_modules(DAVIX_PKG REQUIRED davix)

include_directories(${DAVIX_PKG_INCLUDE_DIRS})

# Configure the target for the consumer shared library that we want to provide
add_library(ugrnoloopplugin SHARED ${src_noloop})

set_target_properties(ugrnoloopplugin        PROPERTIES LINK_FLAGS "-rdynamic"
                                                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/plugins)

target_link_libraries(ugrnoloopplugin "dl" ugrconnector ${DAVIX_PKG_LIBRARIES})


  # How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrnoloopplugin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT plugins-noloop)

