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


# include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(dmlite)
add_subdirectory(dmliteclient)
add_subdirectory(httpclient)
add_subdirectory(davclient)
add_subdirectory(davclient_rucio)
add_subdirectory(s3client)
add_subdirectory(lfcclient)
add_subdirectory(geoIP)
add_subdirectory(noloop)








add_subdirectory(MSAzureclient)