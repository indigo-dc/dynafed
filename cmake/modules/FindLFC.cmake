# This module detects if lfc is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
# 
# LFC_LIBRARIES = full path to the lfc libraries
# LFC_INCLUDE = include dir to be used when using the lfc library
# LFC_FOUND   = set to true if lfc was found successfully
#

if(NOT LCG_HOME)
    set(LCG_HOME /opt/lcg)
endif(NOT LCG_HOME)

# Look for the libraries
find_library(LFC_LIBRARY 
    NAMES liblfc.so 
    HINTS ${LCG_HOME}/lib ${LCG_HOME}/lib64 ${LCG_HOME}/lib32
)
find_library(LCGDM_LIBRARY 
    NAMES liblcgdm.so 
    HINTS ${LCG_HOME}/lib ${LCG_HOME}/lib64 ${LCG_HOME}/lib32
)
set(LFC_LIBRARIES "${LFC_LIBRARY} ${LCGDM_LIBRARY}")

# Look for the include dir
find_path(LFC_INCLUDE NAMES lfc/lfc_api.h HINTS ${LCG_HOME}/include)

# Set the variables documented above if library was found, fail if not
if(LFC_LIBRARY AND LCGDM_LIBRARY AND LFC_INCLUDE)
    if(NOT LFC_FIND_QUIETLY)
        message(STATUS "Found lfc: ${LFC_LIBRARIES}")
        message(STATUS "Found lfc include: ${LFC_INCLUDE}")
    endif(NOT LFC_FIND_QUIETLY)
    set(LFC_FOUND TRUE)
else(LFC_LIBRARY AND LCGDM_LIBRARY AND LFC_INCLUDE)
    if(LFC_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find lfc")
    endif(LFC_FIND_REQUIRED)
endif(LFC_LIBRARY AND LCGDM_LIBRARY AND LFC_INCLUDE)
