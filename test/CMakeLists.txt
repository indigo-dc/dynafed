# main file for src

# force RPATH usage for easy test execution
set(CMAKE_SKIP_RPATH OFF
    CACHE BOOL "" FORCE)


include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src/)
link_directories(${PROJECT_BINARY_DIR}/src/)


add_subdirectory(unit)




