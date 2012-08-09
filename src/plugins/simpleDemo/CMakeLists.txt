cmake_minimum_required (VERSION 2.6)


# The source modules
set(UgrLocPlugin_simpleDemo_SOURCES UgrLocPlugin_simpleDemo.cc)


message(STATUS "Building the Demo plugin")

# Configure the target for the consumer shared library that we want to provide
add_library(ugrlocplugin_simpleDemo SHARED ${UgrLocPlugin_simpleDemo_SOURCES})
SET_TARGET_PROPERTIES(ugrlocplugin_simpleDemo PROPERTIES LINK_FLAGS "-rdynamic")
target_link_libraries(ugrlocplugin_simpleDemo ugrconnector)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrlocplugin_simpleDemo
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  COMPONENT plugins-simpleDemo)

