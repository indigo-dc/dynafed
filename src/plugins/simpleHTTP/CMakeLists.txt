cmake_minimum_required (VERSION 2.6)


# The source modules
set(UgrLocPlugin_simpleHTTP_SOURCES UgrLocPlugin_simpleHTTP.cc)


message(STATUS "Building the HTTP plugin")

# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_simplehttp SHARED ${UgrLocPlugin_simpleHTTP_SOURCES})
SET_TARGET_PROPERTIES(ugrlocplugin_simplehttp PROPERTIES LINK_FLAGS "-rdynamic")
target_link_libraries(ugrlocplugin_simplehttp ugrconnector)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_simplehttp
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  COMPONENT plugins-simplehttp)

