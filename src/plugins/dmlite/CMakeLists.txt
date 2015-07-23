cmake_minimum_required (VERSION 2.6)



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

#
# if dmlite is available then we build also a plugin that it can load, to get Ugr services
#
if(DMLITE_FOUND)

  # Include the dmlite headers
  include_directories(${DMLITE_INCLUDE})

  message(STATUS "Compiling the plugin for dmlite")
  set(ugrdmlite_SOURCES UgrDMLite.cc)
  add_library(ugrdmlite SHARED ${ugrdmlite_SOURCES})

  target_link_libraries(ugrdmlite "dl ${DMLITE_LIBRARIES}" ugrconnector)

set_target_properties(ugrdmlite PROPERTIES LINK_FLAGS "-rdynamic"
                                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)

  # Install the plugin for dmlite
  install(TARGETS ugrdmlite
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
    COMPONENT plugins-dmlite)

  # Install the config files
  if (NOT EXISTS ${SYSCONF_INSTALL_DIR}/ugr/ugrdmlite.conf)
    install(FILES ugrdmlite.conf
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION ${SYSCONF_INSTALL_DIR}/ugr
    COMPONENT plugins-dmlite)
  endif()


  # Install the config files
  if (NOT EXISTS ${APACHE_SITES_INSTALL_DIR}/zlcgdm-ugr-dav.conf)
    install(FILES zlcgdm-ugr-dav.conf
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION ${APACHE_SITES_INSTALL_DIR}
    COMPONENT plugins-dmlite)
  endif()



endif()
