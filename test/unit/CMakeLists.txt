# main file for src

if(UNIT_TESTS)

include_directories(${PROJECT_SOURCE_DIR}/src "." ${GTEST_INCLUDE_DIRS})

FILE(GLOB src_test "*.cpp")


add_executable(g_test_unit_ugr_bin ${src_test})
target_link_libraries(g_test_unit_ugr_bin ugrconnector  gtest gtest_main)
add_dependencies(g_test_unit_ugr_bin ugrconnector)



add_test(g_test_unit_ugr g_test_unit_ugr_bin)

endif(UNIT_TESTS)
