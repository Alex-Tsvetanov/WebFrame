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
    # warning level 4
    add_compile_options(/W4)
else()
    # lots of warnings
    add_compile_options(-Wall -Wextra -pedantic)
endif()

option(USE_INJA "Use Inja" ON)
option(RUN_TESTS "Run unit & integration tests" ON)

configure_file(Core.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/Core-version.hpp)

if (WIN32)
    list(APPEND EXTRA_LIBS wsock32 ws2_32)
endif()

find_package(Threads REQUIRED)
list(APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})

if(CMAKE_USE_PTHREADS_INIT) 
    add_definitions(-DUSE_PTHREAD)
endif()

# Include sub-projects.
if(USE_INJA)
    add_subdirectory("lib/inja")
    add_definitions(-DUSE_INJA)
    list(APPEND EXTRA_LIBS single_inja)
endif()

add_subdirectory("lib/url-matcher")
add_library(Core STATIC "lib/core/core.cpp")
target_link_libraries(Core PUBLIC ${EXTRA_LIBS})
target_include_directories(Core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set_target_properties(Core PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS Core DESTINATION ${PROJECT_BINARY_DIR})

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
