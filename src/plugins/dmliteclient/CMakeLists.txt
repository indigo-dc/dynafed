cmake_minimum_required (VERSION 2.6)




#
# if dmlite is available then we build also a plugin that uses it
#
if(DMLITE_FOUND)


  # Include the dmlite headers
  include_directories(${DMLITE_INCLUDE})

  # The source modules
  set(UgrLocPlugin_dmliteclient_SOURCES UgrLocPlugin_dmliteclient.cc)


  message(STATUS "Building the dmlite client plugin")

  # Configure the target for the consumer shared library that we want to provide
  add_library(ugrlocplugin_dmliteclient SHARED ${UgrLocPlugin_dmliteclient_SOURCES})

  SET_TARGET_PROPERTIES(ugrlocplugin_dmliteclient PROPERTIES LINK_FLAGS "-rdynamic")

  target_link_libraries(ugrlocplugin_dmliteclient "dl ${DMLITE_LIBRARIES}" ugrconnector)


  # How to install. This is a set of plugins that belong to a specific component
  install(TARGETS ugrlocplugin_dmliteclient
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    LIBRARY DESTINATION usr/${LIBDESTINATION} COMPONENT plugins-dmliteclient)

  # Install the config files
  install(FILES ugrdmliteclient.conf
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION /etc
    COMPONENT plugins-dmliteclient)

ENDIF()