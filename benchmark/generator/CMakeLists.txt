# The load generator is deliberately not linked against coroute.
#
# A client built from the server's own library would share its parsing, its buffering
# and its bugs, and a measurement in which both sides can be wrong in the same way is
# not a measurement of either. Cleartext uses nothing but sockets and threads. OpenSSL
# is linked only so the TLS arm of the demultiplexer comparison can speak TLS; the
# certificate is not verified, and nothing from coroute's TLS wrapper is reused.
add_executable(loadgen loadgen.cpp)
target_compile_features(loadgen PRIVATE cxx_std_20)

if(WIN32)
    target_link_libraries(loadgen PRIVATE ws2_32 winmm)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(loadgen PRIVATE Threads::Threads)
endif()

find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_compile_definitions(loadgen PRIVATE LOADGEN_HAS_TLS=1)
    target_link_libraries(loadgen PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "loadgen: TLS arm enabled (OpenSSL ${OPENSSL_VERSION})")
else()
    message(STATUS "loadgen: TLS arm disabled (OpenSSL not found; --tls will refuse)")
endif()

set_target_properties(loadgen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmark")
