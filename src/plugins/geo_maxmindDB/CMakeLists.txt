#
#  Copyright (c) CERN 2017
#
#  Licensed under the Apache License, Version 2.0
#  See the LICENSE file for further information
#




cmake_minimum_required (VERSION 2.6)

#
# if maxminddb is available then we build also a plugin that uses it
# to provide geo translations to ugr replicas
#
if(MMDB_GEO_PLUGIN AND MMDB_GEO_LIB)

  # The source modules
  set(UgrGeoPlugin_mmdb_SOURCES UgrGeoPlugin_mmdb.cc)


  message(STATUS "Building the MaxMind geo plugin")

  # Configure the target for the consumer shared library that we want to provide
  add_library(ugrgeoplugin_mmdb SHARED ${UgrGeoPlugin_mmdb_SOURCES})

set_target_properties(ugrgeoplugin_mmdb         PROPERTIES LINK_FLAGS "-rdynamic"
                                                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)

  target_link_libraries(ugrgeoplugin_mmdb "dl" ugrconnector ${MMDB_GEO_LIB})


  # How to install. This is a set of plugins that belong to a specific component
  install(TARGETS ugrgeoplugin_mmdb
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT plugins-mmdb)

ENDIF(MMDB_GEO_PLUGIN AND MMDB_GEO_LIB)
