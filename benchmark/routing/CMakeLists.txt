# Level one of the routing experiment: dispatch cost with nothing else in the way.
#
# Linked against coroute on purpose, unlike the load generator. What is under test here
# IS the framework's router, so measuring anything else would answer a different
# question than the end-to-end level does.
add_executable(route_bench route_bench.cpp)
target_compile_features(route_bench PRIVATE cxx_std_20)
target_link_libraries(route_bench PRIVATE coroute)
target_include_directories(route_bench PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    target_link_libraries(route_bench PRIVATE psapi)
endif()

set_target_properties(route_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmark")
