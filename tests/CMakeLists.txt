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
    test_deferred.cpp
    test_deferred_stream.cpp
    test_io_context.cpp
    test_http3_cid.cpp
    test_http3_packet.cpp
    test_http3_headers.cpp
    test_http3_stateless.cpp
    test_tls_context.cpp
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
    test_timer_queue.cpp
    test_deadline.cpp
    test_idle_timeout.cpp
    test_request_parse.cpp
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
#
# SKIP_RETURN_CODE 4 is Catch2's "no test case ran to completion", which is what it
# returns when every arm of a test case skipped. On a host with kernel.io_uring_disabled=1
# that is exactly what the io_context and datagram cases do, and without this ctest reads
# the 4 as a failure and the suite is red on every hardened kernel for a reason that is
# not a defect. See tests/io_backend_arms.hpp.
catch_discover_tests(unit_tests PROPERTIES SKIP_RETURN_CODE 4)

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
        # One registration per compiled backend, because these drive a real server
        # process and the backend is a property of that process rather than of a
        # Catch2 run. The unit tests take the other route the same problem allows: a
        # generator over the compiled set inside one binary. See tests/io_backend_arms.hpp.
        #
        # 'host-default' is the sentinel for a platform with one backend and no flag
        # naming it, which is every platform except Linux.
        set(_demux_arms "")
        foreach(_backend io_uring epoll)
            if("${_backend}" IN_LIST _coroute_backends)
                list(APPEND _demux_arms "${_backend}")
            endif()
        endforeach()
        if(NOT _demux_arms)
            set(_demux_arms "host-default")
        endif()

        # Built with if() rather than a generator expression: an unsatisfied
        # $<BOOL:...> genex still yields an empty argv entry, which argparse rejects.
        set(_demux_common "")
        if(COROUTE_ENABLE_TLS)
            list(APPEND _demux_common --tls)
        endif()
        if(COROUTE_ENABLE_HTTP2)
            list(APPEND _demux_common --http2)
        endif()

        # The certificate is one fixture for every arm: it is the same certificate, and
        # making it twice would be two things that can disagree.
        if(COROUTE_ENABLE_TLS)
            set(_tls_certs "${CMAKE_CURRENT_BINARY_DIR}/tls_demux_certs")
            add_test(NAME tls_demux_cert
                COMMAND ${Python3_EXECUTABLE} -m benchmark.make_cert --out ${_tls_certs})
            set_tests_properties(tls_demux_cert PROPERTIES
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                TIMEOUT 60
                FIXTURES_SETUP tls_demux_fixture)
        endif()

        # Ports are handed out per arm rather than shared, so the arms can run
        # concurrently under ctest -j. tls_demux uses three consecutive ports.
        set(_demux_port 18099)
        set(_tls_port 18120)
        foreach(_arm IN LISTS _demux_arms)
            if(_arm STREQUAL "host-default")
                set(_suffix "")
                set(_arm_args "")
            else()
                set(_suffix "_${_arm}")
                set(_arm_args --io-backend ${_arm})
            endif()

            add_test(NAME protocol_demux${_suffix}
                COMMAND ${Python3_EXECUTABLE}
                        ${CMAKE_CURRENT_SOURCE_DIR}/integration/protocol_demux.py
                        $<TARGET_FILE:benchmark_server>
                        --port ${_demux_port}
                        ${_demux_common}
                        ${_arm_args})
            # 77 is the scripts' SKIP_EXIT: the host refused this backend, which is not
            # a defect. Without this a hardened kernel turns the suite red.
            set_tests_properties(protocol_demux${_suffix} PROPERTIES
                TIMEOUT 60
                SKIP_RETURN_CODE 77)
            math(EXPR _demux_port "${_demux_port} + 1")

            # The claim itself, asserted: one descriptor answering both transports, and
            # the two dedicated listeners it is measured against refusing the other one.
            # Only registered where there is a TLS build to assert it about, and it makes
            # its own certificate so a fresh checkout needs no setup step.
            if(COROUTE_ENABLE_TLS)
                add_test(NAME tls_demux${_suffix}
                    COMMAND ${Python3_EXECUTABLE}
                            ${CMAKE_CURRENT_SOURCE_DIR}/integration/tls_demux.py
                            $<TARGET_FILE:benchmark_server>
                            --cert ${_tls_certs}/bench.crt
                            --key ${_tls_certs}/bench.key
                            --port ${_tls_port}
                            ${_arm_args})
                set_tests_properties(tls_demux${_suffix} PROPERTIES
                    TIMEOUT 90
                    FIXTURES_REQUIRED tls_demux_fixture
                    SKIP_RETURN_CODE 77)
                math(EXPR _tls_port "${_tls_port} + 3")
            endif()
        endforeach()

        unset(_demux_common)
        unset(_demux_arms)
        unset(_tls_certs)
    else()
        message(STATUS "Python3 not found: skipping the protocol_demux integration test")
    endif()
endif()
