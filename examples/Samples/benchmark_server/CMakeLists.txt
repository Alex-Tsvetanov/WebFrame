add_executable(benchmark_server main.cpp)
target_link_libraries(benchmark_server PRIVATE coroute)

# The routing experiment's table generator, shared with route_bench and with whatever
# the load generator is pointed at. One generator so the server and the client cannot
# drift onto different tables.
target_include_directories(benchmark_server PRIVATE "${CMAKE_SOURCE_DIR}/benchmark/routing")
