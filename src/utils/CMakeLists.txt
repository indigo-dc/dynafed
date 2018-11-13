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

# tada

install(PROGRAMS      ugrpullscript_gfal.sh
        DESTINATION   ${CMAKE_INSTALL_PREFIX}/libexec/ugr)

install(PROGRAMS      ugrpushscript_gfal.sh
        DESTINATION   ${CMAKE_INSTALL_PREFIX}/libexec/ugr)

