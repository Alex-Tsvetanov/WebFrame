cmake_minimum_required(VERSION 3.14)

add_subdirectory(Catch2-3.1.1)

add_executable(Core-Tests main.cpp)
target_include_directories(Core-Tests PUBLIC "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(Core-Tests PUBLIC Core Catch2::Catch2)

add_test(NAME Usage COMMAND Core-Tests)
set_tests_properties(Usage
  PROPERTIES 
  PASS_REGULAR_EXPRESSION "+ [0-9]+)"
  FAIL_REGULAR_EXPRESSION "x [0-9]+)"
)
