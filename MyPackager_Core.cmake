set(MYPACKAGE_COMPONENTS Core CoreSys)
#set(MYPACKAGE_COMPONENTS Core )

set(CPACK_INSTALL_CMAKE_PROJECTS)
foreach(component ${MYPACKAGE_COMPONENTS})
  list(APPEND CPACK_INSTALL_CMAKE_PROJECTS ".;SEMsg;${component};/")
endforeach(component)

execute_process( COMMAND uname -m
  OUTPUT_VARIABLE ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# The version number
set (SEMsg_VERSION_MAJOR 1)
set (SEMsg_VERSION_MINOR 2)
set (SEMsg_VERSION_PATCH 0beta)

set(CPACK_GENERATOR RPM )
SET(CPACK_CMAKE_GENERATOR "Unix Makefiles")

SET(CPACK_SET_DESTDIR ON)

SET(CPACK_PACKAGE_NAME "semsgcore")
SET(CPACK_PACKAGE_SUMMARY "The SEMsg library: messaging support for Storage Elements")
SET(CPACK_PACKAGE_DESCRIPTION "The SEMsg library: messaging support for Storage Elements")
SET(CPACK_PACKAGE_VENDOR "CERN IT-GT-DMS")
SET(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_HOME_DIRECTORY}/README.txt)
set(CPACK_PACKAGE_CONTACT fabrizio.furano@cern.ch)
set(CPACK_PACKAGE_VERSION "${SEMsg_VERSION_MAJOR}.${SEMsg_VERSION_MINOR}.${SEMsg_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${SEMsg_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${SEMsg_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${SEMsg_VERSION_PATCH}")
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${ARCH}")


#set(CPACK_RPM_SPEC_INSTALL_POST "cp etc/SEMsgdaemon.script /etc/init.d/SEMsgdaemon ; cp ./SEMsgdaemon.sysconfig /etc/sysconfig/SEMsgdaemon")

#SET(CPACK_INSTALL_PREFIX "/opt/lcg")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_TOPLEVEL_TAG "")

set(CPACK_RPM_PACKAGE_DEBUG 1)
set(CPACK_RPM_PACKAGE_ARCHITECTURE ${ARCH})

  
