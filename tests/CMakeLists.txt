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
    test_sha1.cpp
    test_protocol_detect.cpp
    test_datagram.cpp
    test_io_context.cpp
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
)

target_link_libraries(unit_tests PRIVATE coroute Catch2::Catch2WithMain)

# Register tests with CTest
include(CTest)
include(Catch)

# Known limitation, Windows only: a clang sanitizer build links the sanitizer runtime
# as a DLL, and ctest does not put the compiler's runtime directory on the search path
# for the test processes it spawns. Every test then fails to start, while the same
# binary run directly from a shell that has that directory on PATH passes.
#
# The sanitizer presets are aimed at Linux CI, where there is no such runtime DLL and
# ctest works normally. On Windows, run the binary directly to use them:
#
#   PATH="<llvm>/lib/clang/<ver>/lib/windows:$PATH" build/asan/tests/unit_tests.exe
#
# Catch2's DL_PATHS option looks like the fix but did not take effect here; worth
# revisiting if Windows sanitizer runs become part of the routine gate.
catch_discover_tests(unit_tests)

# ============================================================================
# Integration: protocol demultiplexing over one listening socket
# ============================================================================
#
# Needs a real server and real sockets: it tests what the accept path does with the
# first octets of a connection, which no unit test can reach. This is currently the
# only coverage src/core/app.cpp has.
if(COROUTE_BUILD_EXAMPLES)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        # Built with if() rather than a generator expression: an unsatisfied
        # $<BOOL:...> genex still yields an empty argv entry, which argparse rejects.
        set(_demux_args --port 18099)
        if(COROUTE_ENABLE_TLS)
            list(APPEND _demux_args --tls)
        endif()
        if(COROUTE_ENABLE_HTTP2)
            list(APPEND _demux_args --http2)
        endif()

        add_test(NAME protocol_demux
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/integration/protocol_demux.py
                    $<TARGET_FILE:benchmark_server>
                    ${_demux_args})
        set_tests_properties(protocol_demux PROPERTIES TIMEOUT 60)
        unset(_demux_args)
    else()
        message(STATUS "Python3 not found: skipping the protocol_demux integration test")
    endif()
endif()
