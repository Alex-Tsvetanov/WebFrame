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

# OpenSSL is optional here and required nowhere else in this directory. The generator
# without it runs every cleartext campaign; --tls on that build refuses at startup
# rather than connecting in cleartext and letting the record claim otherwise.
#
# Found independently of COROUTE_ENABLE_TLS on purpose. This is the client, and the
# rule that the client must not share the server's library is what makes a difference
# between them attributable to the server. A shared build option would be one more way
# for the two to be wrong together.
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(loadgen PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(loadgen PRIVATE COROUTE_LOADGEN_TLS)
    message(STATUS "loadgen: TLS enabled (OpenSSL ${OPENSSL_VERSION})")
else()
    message(STATUS "loadgen: TLS disabled (no OpenSSL); --tls will refuse to run")
endif()

set_target_properties(loadgen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmark")
