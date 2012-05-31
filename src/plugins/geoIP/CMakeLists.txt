cmake_minimum_required (VERSION 2.6)




#
# if geoip is available then we build also a plugin that uses it
#
if(GEOIP_FOUND)

  # The source modules
  set(UgrGeoPlugin_GeoIP_SOURCES UgrGeoPlugin_GeoIP.cc)


  message(STATUS "Building the GeoIP geo plugin")

  # Configure the target for the consumer shared library that we want to provide
  add_library(ugrgeoplugin_geoip SHARED ${UgrGeoPlugin_GeoIP_SOURCES})

  SET_TARGET_PROPERTIES(ugrgeoplugin_geoip PROPERTIES LINK_FLAGS "-rdynamic")

  target_link_libraries(ugrgeoplugin_geoip "dl ${DMLITE_LIBRARIES}" ugrconnector)


  # How to install. This is a set of plugins that belong to a specific component
  install(TARGETS ugrgeoplugin_geoip
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    LIBRARY DESTINATION ${LIBDESTINATION} COMPONENT plugins-geoip)

ENDIF()
