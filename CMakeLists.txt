cmake_minimum_required(VERSION 3.16)

project(instrucion-scheduler LANGUAGES CXX)

include(cmake/tools.cmake)

include(cmake/defaults.cmake)

include(cmake/third_party.cmake)

add_library(${PROJECT_NAME}_lib
    ${CMAKE_CURRENT_SOURCE_DIR}/source/configs_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dagraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dump.cpp
)

target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(ENABLE_TESTS "Enable testing" ON)
if (ENABLE_TESTS)
  enable_testing()
  include(GoogleTest)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

target_link_libraries(${PROJECT_NAME}_lib PRIVATE instruction-scheduler-defaults PUBLIC tomlplusplus::tomlplusplus)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE instruction-scheduler-defaults ${PROJECT_NAME}_lib cxxopts::cxxopts)

