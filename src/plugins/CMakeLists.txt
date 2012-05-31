cmake_minimum_required (VERSION 2.6)

# include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(1)
#if(LFC_FOUND)
  message(STATUS "Compiling the legacyLFC plugin.")
  add_subdirectory(legacylfc)
else()
  message(STATUS "Skipping the legacyLFC plugin.")
endif()








