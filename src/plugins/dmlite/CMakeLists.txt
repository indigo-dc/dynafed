cmake_minimum_required (VERSION 2.6)


#
# if dmlite is available then we build also a plugin for it
#
if(DMLITE_FOUND)

  # Include the dmlite headers
  include_directories(${DMLITE_INCLUDE})

  message(STATUS "Compiling the plugin for dmlite")
  set(ugrdmlite_SOURCES UgrDMLite.cc)
  add_library(ugrdmlite SHARED ${ugrdmlite_SOURCES})

  target_link_libraries(ugrdmlite "dl ${DMLITE_LIBRARIES}" UgrConnector)

  # Install the plugin for dmlite
  install(TARGETS ugrdmlite
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    LIBRARY DESTINATION ${LIBDESTINATION}
    COMPONENT Core)
endif()
