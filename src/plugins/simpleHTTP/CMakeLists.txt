cmake_minimum_required (VERSION 2.6)


# The source modules
set(UgrLocPlugin_simpleHTTP_SOURCES UgrLocPlugin_simpleHTTP.cc)


message(STATUS "Going to build target UgrLocPlugin_simpleHTTP${__CURRENT_VERSION_NO_DOTS}")
# Configure the target for the consumer shared library that we want to provide
add_library(UgrLocPlugin_simpleHTTP${__CURRENT_VERSION_NO_DOTS} SHARED ${UgrLocPlugin_simpleHTTP_SOURCES})
SET_TARGET_PROPERTIES(UgrLocPlugin_simpleHTTP${__CURRENT_VERSION_NO_DOTS} PROPERTIES LINK_FLAGS "-rdynamic")

#target_link_libraries(UgrLocPlugin_legacylfc${__CURRENT_VERSION_NO_DOTS} UGRConnector)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS UgrLocPlugin_simpleHTTP${__CURRENT_VERSION_NO_DOTS}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION usr/${LIBDESTINATION} COMPONENT Plugins-simpleHTTP)

