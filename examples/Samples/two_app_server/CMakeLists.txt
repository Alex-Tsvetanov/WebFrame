add_executable(two_app_server main.cpp)
target_link_libraries(two_app_server PRIVATE coroute)

# The same table generator the other arm uses, so one App and two cannot be compared
# across two different route tables.
target_include_directories(two_app_server PRIVATE "${CMAKE_SOURCE_DIR}/benchmark/routing")
