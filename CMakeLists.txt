# Core main CMakeLists with all sub projects
cmake_minimum_required (VERSION 3.14)

project ("Core" VERSION 3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-stdlib=libc++)
endif()

if(OPTIMIZATION)
    add_compile_options(${OPTIMIZATION})
endif()

if (MSVC)
    add_compile_options(/await)
    list(APPEND PROJECT_WARNING_LEVEL "/W4")
    list(APPEND DEPENDANCES_WARNING_LEVEL "/W0")
else()
    # lots of warnings
    if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        add_compile_options(-fcoroutines)
    endif()
    list(APPEND PROJECT_WARNING_LEVEL "-Wall" "-Wextra" "-pedantic")
    list(APPEND DEPENDANCES_WARNING_LEVEL "-w")
endif()
add_compile_options(${PROJECT_WARNING_LEVEL})

option(USE_INJA "Use Inja" ON)
option(RUN_TESTS "Run unit & integration tests" ON)

configure_file(Core.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/Core-version.hpp)

add_subdirectory(lib/)

if(RUN_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory("tests/")
endif()

# Generate example source code
add_executable(BenchmarkNormal benchmark/contestants/server.cpp)
target_link_libraries(BenchmarkNormal PUBLIC Core)
target_include_directories(BenchmarkNormal PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          )
                          
add_executable(BenchmarkAtomic benchmark/contestants/server_atomic.cpp)
target_link_libraries(BenchmarkAtomic PUBLIC Core)
target_include_directories(BenchmarkAtomic PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          )

add_executable(SampleExample example/Sample/main.cpp)
target_link_libraries(SampleExample PUBLIC Core)
target_include_directories(SampleExample PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          )

add_executable(ProjectExample example/Project/main.cpp)
target_link_libraries(ProjectExample PUBLIC Core)
target_include_directories(ProjectExample PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          )

include(CPack)
