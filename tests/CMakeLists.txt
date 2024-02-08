cmake_minimum_required(VERSION 3.14)

add_subdirectory(Catch2)
set_target_properties(Catch2 PROPERTIES COMPILE_FLAGS ${DEPENDANCES_WARNING_LEVEL})

add_executable(Core-Tests main.cpp)
target_include_directories(Core-Tests PUBLIC "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(Core-Tests PUBLIC Core Catch2::Catch2)

add_test(NAME Webframe-Core-Tests COMMAND Core-Tests)
