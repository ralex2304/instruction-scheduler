
add_executable(unit_tests ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)

target_link_libraries(unit_tests PRIVATE instruction-scheduler-defaults ${PROJECT_NAME}_lib GTest::gtest_main)

target_compile_definitions(unit_tests PUBLIC TESTS_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

gtest_discover_tests(unit_tests)

