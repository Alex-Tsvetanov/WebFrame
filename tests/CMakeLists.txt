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
# The backend-parametrised cases are registered once per compiled arm, so each arm
# reports its own verdict.
#
# A single registration cannot do it. Catch2 marks a whole case skipped when any
# assertion skipped and none failed (catch_totals.cpp:52-62) and returns 4 when every
# case in the run skipped (catch_session.cpp:345-349); with one case per ctest entry
# that is always the whole run. So in a dual tree the epoll arm ran, passed, and still
# read SKIPPED, which is a green light over an arm nobody has confirmed.
#
# Splitting on TEST_SUFFIX and PROPERTIES alone does not work either: Catch.cmake derives
# its generated tests-file name from a hash of TEST_SPEC, EXTRA_ARGS, REPORTER and the
# OUTPUT_* options only (Catch.cmake:175-181), so two calls differing in suffix and
# environment collide on that file and the second overwrites the first. EXTRA_ARGS is in
# the hash, which is what --name is doing here: it names the suite in the report and,
# incidentally, makes the two registrations distinct.
if("io_uring" IN_LIST _coroute_backends AND "epoll" IN_LIST _coroute_backends)
    # No SKIP_RETURN_CODE. epoll is not restricted by any sysctl, so a skip on this arm
    # means the parametrisation stopped selecting it, and that must fail loudly rather
    # than read as an expected skip.
    catch_discover_tests(unit_tests
        TEST_SPEC "[io_context],[datagram]"
        TEST_SUFFIX _epoll
        EXTRA_ARGS --name unit_tests_epoll
        PROPERTIES ENVIRONMENT COROUTE_TEST_IO_BACKEND=epoll)

    # --abort so a failure exits 1 rather than the failure count. Catch2 returns
    # min(255, failed) (catch_session.cpp:354), so exactly four failed assertions would
    # exit 4 and SKIP_RETURN_CODE would report the run as skipped: a red run wearing a
    # skip's colours, on the one arm where a skip is routine.
    catch_discover_tests(unit_tests
        TEST_SPEC "[io_context],[datagram]"
        TEST_SUFFIX _io_uring
        EXTRA_ARGS --name unit_tests_io_uring --abort
        PROPERTIES ENVIRONMENT COROUTE_TEST_IO_BACKEND=io_uring
                   SKIP_RETURN_CODE 4
                   # Serialised against the other io_uring entries, including the
                   # integration arms below, because io_uring ring memory is charged
                   # against a per-user RLIMIT_MEMLOCK budget: about nine 8192-entry
                   # rings on a default 8MiB limit, shared by every process this user
                   # runs. These tests want more than nine between them.
                   #
                   # This is the second of two mechanisms and neither fixes the other,
                   # which is worth stating because each on its own looks like the
                   # answer. The budget is also released asynchronously, 7 to 14ms after
                   # a process exits, so consecutive tests overlap in accounting while
                   # never overlapping in time; that part is not concurrency at all and a
                   # lock does nothing for it. It is handled by the bounded retry in
                   # tests/io_backend_arms.hpp, and measured: with the retry and no lock,
                   # ctest -j1 passes five runs out of five.
                   #
                   # What the retry cannot fix is genuine simultaneous demand. Two tests
                   # each retrying for their rings hold what they have already taken
                   # while waiting for the rest, so they starve each other and both time
                   # out. With the retry and no lock, ctest -j16 failed three runs in
                   # five. Hence both.
                   RESOURCE_LOCK io_uring_memlock)

    # Everything else, once, with no backend in play.
    catch_discover_tests(unit_tests TEST_SPEC "~[io_context]~[datagram]")
else()
    # One backend compiled in, so there is nothing to split and nothing sets
    # COROUTE_TEST_IO_BACKEND. A case can still skip, on a single-backend io_uring build
    # against a hardened kernel, and 4 is how Catch2 says so.
    #
    # Only there. io_backend_probe returns 0 unconditionally for Epoll and for Default
    # (io_context.cpp), and Default is the only arm io_backend_arms() yields on iocp and
    # kqueue, so on Windows, macOS and a single-backend epoll build nothing in the suite
    # can reach the one SKIP it has. The property then has no use and one failure mode:
    # Catch2 returns min(255, failed assertions), so a case with exactly four failing
    # CHECKs exits 4, ctest prints it as Skipped, and the run reports 100% pass over a
    # red case. Same hazard the io_uring arm above spends --abort on, and the same
    # answer: do not claim 4 means skipped where a skip cannot happen.
    #
    # --abort for the same reason the dual io_uring arm above uses it: this is now the
    # only registration that still claims 4 means skipped, so it is the only one where
    # exactly four failed assertions could exit 4 and print green.
    if("io_uring" IN_LIST _coroute_backends)
        catch_discover_tests(unit_tests EXTRA_ARGS --abort
            PROPERTIES SKIP_RETURN_CODE 4 RESOURCE_LOCK io_uring_memlock)
    else()
        catch_discover_tests(unit_tests)
    endif()
endif()

# ============================================================================
# Census: the tests this build did not get
# ============================================================================
#
# A feature guard around a whole test file does not make its tests skip. It makes them
# absent, and Catch2 cannot report what it was never given, so the run passes and the
# exit status is zero. Eleven HTTP/3 tests had never executed in any configuration and
# no output had ever said so. This prints the count on every run and fails when a case
# the sources define is missing with no undefined macro to account for it.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(NAME test_census
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_census.py
                --binary $<TARGET_FILE:unit_tests>
                --source-dir ${CMAKE_CURRENT_SOURCE_DIR}
                # The definitions the library actually carries, rather than the options
                # that were asked for: a feature can be requested and then switched off
                # by a missing dependency, and it is the former that reaches the guards.
                --defines $<TARGET_PROPERTY:coroute,INTERFACE_COMPILE_DEFINITIONS>)
else()
    message(STATUS "Python3 not found: skipping the test census")
endif()

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
            # Same per-user budget and the same lock name as the unit arm: these spawn
            # real servers, and the budget does not care which process spends it.
            if(_arm STREQUAL "io_uring")
                set_tests_properties(protocol_demux${_suffix} PROPERTIES
                    RESOURCE_LOCK io_uring_memlock)
            endif()
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
                # As above. This one runs three servers of two workers, six of the
                # user's nine rings, so it fits alone and beside nothing else.
                if(_arm STREQUAL "io_uring")
                    set_tests_properties(tls_demux${_suffix} PROPERTIES
                        RESOURCE_LOCK io_uring_memlock)
                endif()
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
