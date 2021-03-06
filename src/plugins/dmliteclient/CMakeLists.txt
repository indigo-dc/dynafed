#
#  Copyright (c) CERN 2013
#
#  Copyright (c) Members of the EMI Collaboration. 2011-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
#
#  Licensed under the Apache License, Version 2.0
#  See the LICENSE file for further information
#









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

set_target_properties(ugrlocplugin_dmliteclient PROPERTIES LINK_FLAGS "-rdynamic"
                                                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)

  target_link_libraries(ugrlocplugin_dmliteclient dl ${DMLITE_LIBRARIES} ugrconnector rt)


  # How to install. This is a set of plugins that belong to a specific component
  install(TARGETS ugrlocplugin_dmliteclient
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT plugins-dmliteclient)

  # Install the config files
  if (NOT EXISTS ${SYSCONF_INSTALL_DIR}/ugr/ugrdmliteclient.conf)
    install(FILES ugrdmliteclientMY.conf ugrdmliteclientORA.conf
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
      DESTINATION ${SYSCONF_INSTALL_DIR}/ugr
      COMPONENT plugins-dmliteclient)
  endif()

ENDIF()
