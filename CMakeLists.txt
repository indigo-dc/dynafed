cmake_minimum_required (VERSION 2.6)
project (Ugr)

# build type
set(CMAKE_BUILD_TYPE "RelWithDebInfo"
    CACHE STRING "build type")

# The version number
set (UGR_VERSION_MAJOR 0)
set (UGR_VERSION_MINOR 0)
set (UGR_VERSION_PATCH 0)
execute_process( COMMAND uname -m
  OUTPUT_VARIABLE ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE)


# Build the Doxygen cfg
configure_file (
	       "${PROJECT_SOURCE_DIR}/Doxyfile.in"
	       "${PROJECT_BINARY_DIR}/Doxyfile.cfg"
)

# add configure build options
SET(PLUGIN_WEBDAV TRUE 
	CACHE STRING "enable compilation of the Webdav plugin")




# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (eg MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# import common cmake macros
include(DefineInstallationPaths REQUIRED)
include(ReleaseDebugAutoFlags REQUIRED)
include(CMakeGeneratePkgConfig REQUIRED)
 
# Force the build dorectory to be different from the source directory
include(MacroOutOfSourceBuild)
macro_ensure_out_of_source_build("$(PROJECT_NAME) requires an out of source build.")

# The build type... debug, distribution
#set(CMAKE_BUILD_TYPE Debug)

# fetch from DefineInstallationPaths, std path for packaging
set(LIBDESTINATION "${LIB_INSTALL_DIR}" CACHE STRING " lib path")





# -----------------------
# Look for the external packages that are needed, for both the core and the plugins
find_package( Boost 1.41.0 COMPONENTS date_time filesystem system thread)

# Look for the LFC client
find_package(LFC 1.8.0)

# Look for libDMlite
find_package(DMLite 0.0)
# ----------------------

# generic pkgconfig module for Davix & co
find_package(PkgConfig REQUIRED)

#MaxMind GeoIP
find_package(GeoIP)





add_subdirectory(src)

enable_testing()

ADD_CUSTOM_TARGET(doc
  doxygen "${PROJECT_BINARY_DIR}/Doxyfile.cfg")

#######################################################
# Configure an 'uninstall' target
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

############# CPack section ################
# NOTE... this custom kludge is needed because
# by now CPack is not able to create multiple packages
# following the COMPONENT directives

ADD_CUSTOM_TARGET(package
  COMMAND make
  COMMAND cpack --verbose --config ${CMAKE_HOME_DIRECTORY}/MyPackager_Core.cmake 
    -D CPACK_PACKAGE_DESCRIPTION_FILE=${CMAKE_HOME_DIRECTORY}/README.txt
    -D CMAKE_HOME_DIRECTORY=${CMAKE_HOME_DIRECTORY}
    -D CPACK_INSTALL_PREFIX=${CPACK_INSTALL_PREFIX}
  DEPENDS 
  COMMENT "Building packages..."
  VERBATIM)
