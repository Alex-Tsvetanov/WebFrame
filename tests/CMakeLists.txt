# Tests CMakeLists.txt

# Find or fetch Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Unit tests
add_executable(unit_tests
    test_expected.cpp
    test_error.cpp
    test_from_string.cpp
    test_time.cpp
    test_router.cpp
    test_response.cpp
    test_static_files.cpp
    test_chunked.cpp
    test_compression.cpp
    test_range.cpp
    test_object_pool.cpp
    test_connection_pool.cpp
    test_auth_state.cpp
    http2_tests.cpp
    test_view.cpp
    test_task.cpp
    test_http2_advanced.cpp
    test_udp_socket.cpp
)

target_link_libraries(unit_tests PRIVATE coroute Catch2::Catch2WithMain)

# HTTP/3 tests and in-process test client (only when HTTP/3 is enabled)
if(HTTP3_ENABLED)
    target_sources(unit_tests PRIVATE
        test_http3.cpp
        test_http3_e2e.cpp
        test_http3_retry_token_hmac.cpp
        test_http3_replay_token.cpp
        test_http3_hardening.cpp
        test_http3_datagrams.cpp
        http3/quic_test_client.cpp)
    target_include_directories(unit_tests PRIVATE http3)
    # Path to a pre-generated self-signed test cert (used by E2E tests).
    target_compile_definitions(unit_tests PRIVATE
        COROUTE_TEST_CERT_FILE="${CMAKE_SOURCE_DIR}/examples/Samples/https_server/certs/cert.pem"
        COROUTE_TEST_KEY_FILE="${CMAKE_SOURCE_DIR}/examples/Samples/https_server/certs/key.pem")
endif()

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
