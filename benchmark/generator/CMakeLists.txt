# The load generator is deliberately not linked against coroute.
#
# A client built from the server's own library would share its parsing, its buffering
# and its bugs, and a measurement in which both sides can be wrong in the same way is
# not a measurement of either. This uses nothing but sockets and threads.
add_executable(loadgen loadgen.cpp)
target_compile_features(loadgen PRIVATE cxx_std_20)

if(WIN32)
    target_link_libraries(loadgen PRIVATE ws2_32 winmm)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(loadgen PRIVATE Threads::Threads)
endif()

set_target_properties(loadgen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmark")
