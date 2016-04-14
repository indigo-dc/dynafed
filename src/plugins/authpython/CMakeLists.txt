
#
#  Copyright (c) CERN 2015
#
#
#  Licensed under the Apache License, Version 2.0
#  See the LICENSE file for further information
#


 
cmake_minimum_required (VERSION 2.6)


#
# if geoip is available then we build also a plugin that uses it
#
if(AUTHPYTHON_PLUGIN AND PYTHONLIBS_FOUND)


include_directories(BEFORE ${PYTHON_INCLUDE_PATH})

# The source modules
set(UgrAuthPlugin_python_SOURCES UgrAuthPlugin_python.cc)

STRING(REPLACE "." "" __CURRENT_VERSION_NO_DOTS ${PythonLibs_VERSION})

message(STATUS "Build the Python authorization plugin ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS}")


# Configure the target for the consumer shared library that we want to provide
add_library(ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS} SHARED ${UgrAuthPlugin_python_SOURCES})

SET_TARGET_PROPERTIES(ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS} PROPERTIES LINK_FLAGS "-rdynamic"
                      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/ugr)
                      
target_link_libraries(ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS} "dl" ugrconnector)
target_link_libraries(ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS} ${PYTHON_LIBRARIES})
target_link_libraries(ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS} util)


# How to install. This is a set of plugins that belong to a specific component
install(TARGETS ugrauthplugin_python${__CURRENT_VERSION_NO_DOTS}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT plugins-authpython)

# Install the example python auth script
if (NOT EXISTS ${SYSCONF_INSTALL_DIR}/ugr/conf.d)
  install(FILES ugrauth_example.py
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${SYSCONF_INSTALL_DIR}/ugr/conf.d
  COMPONENT plugins-authpython)
endif()

endif(AUTHPYTHON_PLUGIN AND PYTHONLIBS_FOUND)