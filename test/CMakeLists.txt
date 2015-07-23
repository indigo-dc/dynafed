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


# main file for src

# force RPATH usage for easy test execution
set(CMAKE_SKIP_RPATH OFF
    CACHE BOOL "" FORCE)


include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src/)
link_directories(${PROJECT_BINARY_DIR}/src/)


add_subdirectory(unit)




