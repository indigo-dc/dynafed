cmake_minimum_required (VERSION 2.6)

add_executable(teststat "teststat.cc")
set_target_properties(teststat PROPERTIES COMPILE_FLAGS "-I../")

target_link_libraries (teststat ugrconnector)

add_executable(teststatseq "teststat_sequential.cc")
set_target_properties(teststatseq PROPERTIES COMPILE_FLAGS "-I../")

target_link_libraries (teststatseq ugrconnector)

add_executable(testlistdir "testlistdir.cc")
set_target_properties(teststat PROPERTIES COMPILE_FLAGS "-I../")

target_link_libraries (testlistdir ugrconnector)

add_executable(testlocate "testlocate.cc")
set_target_properties(testlocate PROPERTIES COMPILE_FLAGS "-I../")

target_link_libraries (testlocate ugrconnector)


# How to install. This is part of the Core component
#install(TARGETS teststat
#  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#  DESTINATION ${BIN_INSTALL_DIR} COMPONENT core)


