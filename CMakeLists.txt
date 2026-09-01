cmake_minimum_required(VERSION 3.20)
project(coroute VERSION 2.0.0 LANGUAGES CXX)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(COROUTE_BUILD_EXAMPLES "Build example applications" ON)
option(COROUTE_BUILD_TESTS "Build tests" ON)
option(COROUTE_ENABLE_BROTLI "Enable Brotli compression support" OFF)
option(COROUTE_ENABLE_SIMDJSON "Enable simdjson for fast JSON parsing" ON)
option(COROUTE_ENABLE_TLS "Enable TLS/SSL support via OpenSSL" ON)
option(COROUTE_ENABLE_HTTP2 "Enable HTTP/2 support via nghttp2" ON)
option(COROUTE_ENABLE_HTTP3 "Enable HTTP/3 support via ngtcp2 and nghttp3 (needs OpenSSL 3.5+)" OFF)

# Detect platform and set I/O backend
if(WIN32)
    set(COROUTE_IO_BACKEND "iocp" CACHE STRING "I/O backend (iocp)")
    add_compile_definitions(COROUTE_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    # Linux has two: io_uring by default, epoll selectable with
    # -DCOROUTE_IO_BACKEND=epoll. Both are kept so the completion and readiness
    # models can be compared with everything else held constant, and because
    # io_uring is restricted in enough sandboxes to need a fallback.
    set(COROUTE_IO_BACKEND "io_uring" CACHE STRING "I/O backend (io_uring or epoll)")
    set_property(CACHE COROUTE_IO_BACKEND PROPERTY STRINGS "io_uring" "epoll")
    add_compile_definitions(COROUTE_PLATFORM_LINUX)
elseif(APPLE)
    set(COROUTE_IO_BACKEND "kqueue" CACHE STRING "I/O backend (kqueue)")
    add_compile_definitions(COROUTE_PLATFORM_MACOS)
endif()

message(STATUS "Coroute v${PROJECT_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "I/O Backend: ${COROUTE_IO_BACKEND}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# std::expected.
#
# Switching from the bundled coroute::expected to std::expected changes the type used at
# roughly 200 call sites, and the bundled one is deliberately more permissive: it accepts
# expected<void, E>{}, allows implicit conversions the standard type does not, and deduces
# unexpected differently. That migration is its own piece of work, so it is opt-in. A C++
# standard bump and an expected swap must never land in the same commit.
option(COROUTE_USE_STD_EXPECTED "Use std::expected instead of the bundled implementation" OFF)

if(COROUTE_USE_STD_EXPECTED)
    # CMP0067 is NEW at cmake_minimum_required 3.20, so this try-compile inherits
    # CMAKE_CXX_STANDARD. Under C++20 it silently failed and selected the fallback.
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
        #include <expected>
        int main() {
            std::expected<int, int> e = 42;
            return e.value();
        }
    "    COROUTE_STD_EXPECTED_USABLE)

    if(NOT COROUTE_STD_EXPECTED_USABLE)
        message(FATAL_ERROR
            "COROUTE_USE_STD_EXPECTED=ON but <expected> is not usable with "
            "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} at C++${CMAKE_CXX_STANDARD}.")
    endif()

    add_compile_definitions(COROUTE_HAS_STD_EXPECTED)
    message(STATUS "expected: std::expected (C++23)")
else()
    message(STATUS "expected: bundled coroute::expected")
endif()

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# MinGW builds link libstdc++/libgcc dynamically by default, so any process launched
# without the toolchain bin directory on PATH dies with STATUS_ENTRYPOINT_NOT_FOUND.
# That breaks catch_discover_tests, which runs the test binary at build time to
# enumerate cases, and it fails before a single test has run.
if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++)
endif()

# ============================================================================
# Build gates: sanitizers, coverage, clang-tidy
# ============================================================================
#
# These used to exist only as CI-side CMAKE_CXX_FLAGS strings, which meant they could
# not be reproduced locally without copying a shell line out of a workflow file. They
# belong in the build system so CMakePresets can select them by name.

set(COROUTE_SANITIZER "" CACHE STRING "Sanitizer to enable (address+undefined, thread, memory)")
set_property(CACHE COROUTE_SANITIZER PROPERTY STRINGS "" "address" "undefined" "address+undefined" "thread" "memory")

if(COROUTE_SANITIZER)
    if(MSVC)
        # MSVC only ships AddressSanitizer.
        if(NOT COROUTE_SANITIZER STREQUAL "address")
            message(FATAL_ERROR "MSVC supports only COROUTE_SANITIZER=address, got '${COROUTE_SANITIZER}'")
        endif()
        add_compile_options(/fsanitize=address)
    else()
        string(REPLACE "+" "," _coroute_san_list "${COROUTE_SANITIZER}")

        # memory requires every dependency to be instrumented too. Uninstrumented
        # OpenSSL/nghttp2/zlib/simdjson produce false positives that drown the real
        # findings, so MSan is only meaningful on a core-only configuration.
        if(_coroute_san_list MATCHES "memory")
            if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                message(FATAL_ERROR "COROUTE_SANITIZER=memory requires Clang")
            endif()
            if(COROUTE_ENABLE_TLS OR COROUTE_ENABLE_HTTP2 OR COROUTE_ENABLE_SIMDJSON)
                message(WARNING
                    "MemorySanitizer with uninstrumented third-party libraries reports false "
                    "positives. Configure with TLS, HTTP2 and SIMDJSON OFF for usable output.")
            endif()
            add_compile_options(-fsanitize-memory-track-origins=2)
        endif()

        add_compile_options(-fsanitize=${_coroute_san_list} -fno-omit-frame-pointer -fno-sanitize-recover=all -g)
        add_link_options(-fsanitize=${_coroute_san_list})
        unset(_coroute_san_list)
    endif()
    message(STATUS "Sanitizer: ${COROUTE_SANITIZER}")
endif()

option(COROUTE_ENABLE_COVERAGE "Instrument for coverage" OFF)
if(COROUTE_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Source-based coverage, not gcov: gcov mangles line mapping for coroutines
        # and heavily templated code, both of which this codebase is full of.
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    else()
        message(FATAL_ERROR "COROUTE_ENABLE_COVERAGE requires Clang or GCC")
    endif()
    message(STATUS "Coverage: enabled")
endif()

option(COROUTE_ENABLE_CLANG_TIDY "Run clang-tidy over the library sources" OFF)

# Main library
add_library(coroute STATIC
    # Core
    src/core/error.cpp
    src/core/time.cpp
    src/core/request.cpp
    src/core/response.cpp
    src/core/router.cpp
    src/core/app.cpp
    src/core/static_files.cpp
    src/core/chunked.cpp
    src/core/compression.cpp
    src/core/range.cpp
    src/core/request_parse.cpp
    src/core/json.cpp
    src/core/form.cpp
    src/core/cookie.cpp
    src/core/session.cpp
    src/core/logging.cpp
    src/core/metrics.cpp

    # Network abstraction
    src/net/accept_pool.cpp
    src/net/protocol_detect.cpp
    src/net/websocket.cpp
)

target_include_directories(coroute PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Scope clang-tidy to this target. Setting CMAKE_CXX_CLANG_TIDY globally would also
# lint every FetchContent dependency, which is neither our code nor our problem.
if(COROUTE_ENABLE_CLANG_TIDY)
    find_program(COROUTE_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
    set_target_properties(coroute PROPERTIES
        CXX_CLANG_TIDY "${COROUTE_CLANG_TIDY_EXE};--extra-arg=-std=c++23")
    message(STATUS "clang-tidy: ${COROUTE_CLANG_TIDY_EXE}")
endif()

# Platform-specific I/O implementation
if(COROUTE_IO_BACKEND STREQUAL "iocp")
    target_sources(coroute PRIVATE src/net/iocp/iocp_context.cpp)
    target_link_libraries(coroute PRIVATE ws2_32 mswsock)
    # Prevent Windows.h from defining min/max macros
    target_compile_definitions(coroute PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
elseif(COROUTE_IO_BACKEND STREQUAL "epoll")
    target_sources(coroute PRIVATE src/net/epoll/epoll_context.cpp)
    target_compile_definitions(coroute PRIVATE COROUTE_BACKEND_EPOLL)
elseif(COROUTE_IO_BACKEND STREQUAL "io_uring")
    target_sources(coroute PRIVATE src/net/io_uring/uring_context.cpp)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(URING REQUIRED liburing)
    target_link_libraries(coroute PRIVATE ${URING_LIBRARIES})
    target_include_directories(coroute PRIVATE ${URING_INCLUDE_DIRS})
elseif(COROUTE_IO_BACKEND STREQUAL "kqueue")
    target_sources(coroute PRIVATE src/net/kqueue/kqueue_context.cpp)
else()
    # Fail here rather than at link time. Without a backend nothing defines
    # IoContext::create, and the resulting "undefined reference" points at app.cpp
    # rather than at the misspelled option that caused it.
    message(FATAL_ERROR
        "Unknown COROUTE_IO_BACKEND '${COROUTE_IO_BACKEND}'. "
        "Expected one of: iocp (Windows), io_uring or epoll (Linux), kqueue (macOS).")
endif()

# Fetch url-matcher (RegexMatcher) for DFA-based routing
include(FetchContent)

# The one dependency here that tracks a branch rather than a tag, and also the one the
# routing measurements are a measurement OF. A campaign that recorded "fetched at main"
# recorded nothing: main moves, and a reader re-running it later gets a different
# matcher and different numbers with nothing to say so. Overridable so a measurement can
# name the exact commit it ran against; the default is still the branch, so an ordinary
# build keeps tracking upstream.
# Pinned, not floating. A moving ref made HEAD stop building once already: the tag
# resolved to a newer upstream commit than the one the tree had been written
# against, and a rebuild that touched nothing fetched it. For a dissertation the
# stronger reason is that a measurement has to be reproducible from the recorded
# commit, and it is not if a dependency can change underneath that commit.
set(COROUTE_URL_MATCHER_TAG "d16f30a81158d620e5a5514087b175a2251e4fa3" CACHE STRING
    "Git ref for the RegexMatcher dependency; set to a commit SHA to pin a measurement")

# A shallow clone can only ask for a named ref on some servers, so it is turned off when
# the ref is a raw commit. Left on otherwise, since that is the common case and the
# clone is the slow part of a cold configure.
if(COROUTE_URL_MATCHER_TAG MATCHES "^[0-9a-fA-F]{40}$")
    set(_url_matcher_shallow FALSE)
else()
    set(_url_matcher_shallow TRUE)
endif()

FetchContent_Declare(
    url_matcher
    GIT_REPOSITORY https://github.com/cpp-for-everything/RegexMatcher.git
    GIT_TAG ${COROUTE_URL_MATCHER_TAG}
    GIT_SHALLOW ${_url_matcher_shallow}
    SOURCE_SUBDIR include # Only include the library, skip tests
)
FetchContent_MakeAvailable(url_matcher)
target_link_libraries(coroute PUBLIC RegexMatcher)
message(STATUS "url-matcher: fetched from GitHub")

# Fetch inja (C++ Jinja2 template engine)
FetchContent_Declare(
    inja
    GIT_REPOSITORY https://github.com/pantor/inja.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)
set(INJA_USE_EMBEDDED_JSON ON CACHE BOOL "" FORCE)
set(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INJA_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(inja)
target_link_libraries(coroute PUBLIC inja)
target_compile_definitions(coroute PUBLIC COROUTE_HAS_TEMPLATES)
target_sources(coroute PRIVATE src/view/deferred_stream.cpp)
message(STATUS "inja template engine: enabled")

# Compression dependencies (zlib)
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    message(STATUS "zlib not found - fetching from source")
    include(FetchContent)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
    )
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(zlib)
    target_link_libraries(coroute PRIVATE zlibstatic)
    target_include_directories(coroute PRIVATE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
else()
    message(STATUS "Using system zlib")
    target_link_libraries(coroute PRIVATE ZLIB::ZLIB)
endif()

# Optional Brotli support
if(COROUTE_ENABLE_BROTLI)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(BROTLI libbrotlienc libbrotlidec)
    endif()

    if(BROTLI_FOUND)
        target_compile_definitions(coroute PRIVATE COROUTE_HAS_BROTLI)
        target_link_libraries(coroute PRIVATE ${BROTLI_LIBRARIES})
        target_include_directories(coroute PRIVATE ${BROTLI_INCLUDE_DIRS})
        message(STATUS "Brotli compression: enabled")
    else()
        message(WARNING "Brotli requested but not found - disabling")
    endif()
else()
    message(STATUS "Brotli compression: disabled (use -DCOROUTE_ENABLE_BROTLI=ON to enable)")
endif()

# Optional simdjson support for fast JSON parsing
if(COROUTE_ENABLE_SIMDJSON)
    include(FetchContent)
    FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v3.10.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(simdjson)
    target_link_libraries(coroute PUBLIC simdjson)
    target_compile_definitions(coroute PUBLIC COROUTE_HAS_SIMDJSON)
    message(STATUS "simdjson: enabled")
else()
    message(STATUS "simdjson: disabled (use -DCOROUTE_ENABLE_SIMDJSON=ON to enable)")
endif()

# TLS/SSL via OpenSSL.
#
# COROUTE_ENABLE_TLS used to be declared and then ignored: OpenSSL was found REQUIRED
# unconditionally, so -DCOROUTE_ENABLE_TLS=OFF did nothing and the tree would not
# configure at all without OpenSSL development headers.
if(COROUTE_ENABLE_TLS)
    if(COROUTE_ENABLE_HTTP3)
        # ngtcp2's OpenSSL backend uses the QUIC TLS API that first appeared in 3.5.
        # Distributions are behind: Ubuntu 24.04 ships 3.0.13 with no newer candidate,
        # so this usually means pointing OPENSSL_ROOT_DIR at a locally built 3.5.
        find_package(OpenSSL 3.5 REQUIRED)
    else()
        find_package(OpenSSL REQUIRED)
    endif()
    target_link_libraries(coroute PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(coroute PUBLIC COROUTE_HAS_TLS)
    target_sources(coroute PRIVATE src/net/tls/tls_context.cpp)
    message(STATUS "TLS/SSL: enabled (OpenSSL ${OPENSSL_VERSION})")
else()
    message(STATUS "TLS/SSL: disabled (use -DCOROUTE_ENABLE_TLS=ON to enable)")
endif()

# Optional HTTP/2 support via nghttp2
if(COROUTE_ENABLE_HTTP2)
    include(FetchContent)

    # Fetch nghttp2
    FetchContent_Declare(
        nghttp2
        GIT_REPOSITORY https://github.com/nghttp2/nghttp2.git
        GIT_TAG v1.64.0
        GIT_SHALLOW TRUE
    )

    # Configure nghttp2 build options - must be set BEFORE FetchContent_MakeAvailable
    set(ENABLE_LIB_ONLY ON CACHE BOOL "" FORCE)
    set(ENABLE_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(ENABLE_SHARED_LIB ON CACHE BOOL "" FORCE) # Need this for alias to work
    set(ENABLE_DOC OFF CACHE BOOL "" FORCE)
    set(ENABLE_APP OFF CACHE BOOL "" FORCE)
    set(ENABLE_HPACK_TOOLS OFF CACHE BOOL "" FORCE)
    set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ENABLE_FAILMALLOC OFF CACHE BOOL "" FORCE)
    set(ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    set(WITH_LIBXML2 OFF CACHE BOOL "" FORCE)
    set(WITH_JEMALLOC OFF CACHE BOOL "" FORCE)
    set(WITH_LIBBPF OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(nghttp2)

    # Link against nghttp2 library
    if(TARGET nghttp2_static)
        target_link_libraries(coroute PUBLIC nghttp2_static)
    elseif(TARGET nghttp2)
        target_link_libraries(coroute PUBLIC nghttp2)
    else()
        message(FATAL_ERROR "nghttp2 library target not found")
    endif()

    target_include_directories(coroute PUBLIC
        ${nghttp2_SOURCE_DIR}/lib/includes
        ${nghttp2_BINARY_DIR}/lib/includes
    )
    target_compile_definitions(coroute PUBLIC COROUTE_HAS_HTTP2)

    # Add HTTP/2 source files
    target_sources(coroute PRIVATE
        src/http2/frame.cpp
        src/http2/hpack.cpp
        src/http2/stream.cpp
        src/http2/connection.cpp
    )

    message(STATUS "HTTP/2: enabled (nghttp2)")
else()
    message(STATUS "HTTP/2: disabled (use -DCOROUTE_ENABLE_HTTP2=ON to enable)")
endif()

# ============================================================================
# Optional HTTP/3 support via ngtcp2 + nghttp3
# ============================================================================
#
# ngtcp2 and nghttp3 are protocol state machines only: neither owns a socket nor an
# event loop. That is why they were chosen over msquic or lsquic, which bring their
# own I/O layer and would compete with the IoContext backends for the descriptor.
# Here they sit on top of DatagramSocket like any other protocol.
#
# Found rather than fetched, deliberately. FetchContent cannot compose these: nghttp2,
# nghttp3 and ngtcp2 each call add_custom_target(check) unconditionally, outside any
# BUILD_TESTING guard, and CMake targets are global. Pulling in a second one fails
# with "another target with the same name already exists" and there is no option that
# turns it off. ngtcp2 and nghttp3 additionally collide on a "main" test executable.
#
# scripts/build-quic-deps.sh builds OpenSSL 3.5, nghttp3 and ngtcp2 into one prefix.
# Point CMAKE_PREFIX_PATH and OPENSSL_ROOT_DIR at it. This is also how curl consumes
# them, so it is the well-trodden path rather than a workaround.
if(COROUTE_ENABLE_HTTP3)
    if(NOT COROUTE_ENABLE_TLS)
        message(FATAL_ERROR "COROUTE_ENABLE_HTTP3 requires COROUTE_ENABLE_TLS: QUIC has no cleartext mode.")
    endif()

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NGHTTP3 REQUIRED IMPORTED_TARGET libnghttp3)
    pkg_check_modules(NGTCP2 REQUIRED IMPORTED_TARGET libngtcp2)
    pkg_check_modules(NGTCP2_CRYPTO REQUIRED IMPORTED_TARGET libngtcp2_crypto_ossl)

    # Order matters with static archives: the linker resolves left to right, so a
    # library must appear before the ones it depends on. libngtcp2_crypto_ossl calls
    # into libngtcp2, so listing ngtcp2 first leaves its symbols unresolved.
    target_link_libraries(coroute PUBLIC
        PkgConfig::NGTCP2_CRYPTO
        PkgConfig::NGTCP2
        PkgConfig::NGHTTP3
    )

    target_sources(coroute PRIVATE
        src/http3/cid.cpp
        src/http3/packet.cpp
        src/http3/headers.cpp
        src/http3/stateless.cpp
        src/http3/connection.cpp
        src/http3/endpoint.cpp
    )

    target_compile_definitions(coroute PUBLIC COROUTE_HAS_HTTP3)
    message(STATUS "HTTP/3: enabled (ngtcp2 ${NGTCP2_VERSION}, nghttp3 ${NGHTTP3_VERSION}, OpenSSL ${OPENSSL_VERSION})")
else()
    message(STATUS "HTTP/3: disabled (use -DCOROUTE_ENABLE_HTTP3=ON to enable)")
endif()

# Examples
if(COROUTE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(COROUTE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Alternative router backends, for the routing experiment only.
#
# Off by default: they exist to be measured against, not to be shipped. Turning this on
# puts all three routers in one binary and lets --router pick between them at runtime,
# which is the only arrangement in which a difference smaller than build-to-build
# variation (documented at 5-10%) can be attributed to the router rather than to the
# link order. Two separately compiled binaries cannot answer the question.
option(COROUTE_ROUTER_ARMS "Compile the radix and sequential-regex routers alongside the DFA one" OFF)
if(COROUTE_ROUTER_ARMS)
    include(FetchContent)
    # Crow, for its Trie: the compressed prefix tree its own router dispatches with.
    # A named implementation a reader can go and read, rather than a hand-rolled one.
    # SOURCE_SUBDIR points at a directory with no CMakeLists.txt, so the content is
    # downloaded and nothing of Crow's or asio's own build is configured. Both are
    # header-only; only the include trees are wanted.
    FetchContent_Declare(
        crow_router
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.2.1
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR include
    )
    # Crow's headers include asio even for the parts that do no I/O, so asio comes along.
    FetchContent_Declare(
        asio_headers
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR asio/include
    )
    FetchContent_MakeAvailable(crow_router asio_headers)

    target_sources(coroute PRIVATE src/core/router_arms.cpp)
    target_compile_definitions(coroute PRIVATE COROUTE_ROUTER_ARMS ASIO_STANDALONE)
    target_include_directories(coroute PRIVATE
        ${crow_router_SOURCE_DIR}/include
        ${asio_headers_SOURCE_DIR}/asio/include)
    message(STATUS "router arms: dfa + radix (Crow v1.2.1 Trie) + sequential std::regex")
endif()

# The load generator for chapter VI. Built by default because a measurement tool that
# has to be remembered separately is a measurement tool that is out of date.
option(COROUTE_BUILD_BENCHMARK_TOOLS "Build the load generator" ON)
if(COROUTE_BUILD_BENCHMARK_TOOLS)
    add_subdirectory(benchmark/generator)
    if(COROUTE_ROUTER_ARMS)
        # route_bench is x86 only, and deliberately so. It times with rdtsc because a
        # radix lookup lands near 100 ns and the platform steady_clock quantises at
        # 100 ns, so the clock would blur the very arm the experiment exists to
        # resolve. There is no portable substitute: on Apple Silicon the userspace
        # counter runs off the 24 MHz timebase at 41.67 ns per tick, so a port would
        # compile and then report a coarser histogram than the one it is being
        # compared against.
        #
        # So the target is skipped rather than ported. Dispatch-only numbers come from
        # x86 hosts, and a non-x86 host contributes the end-to-end level instead, where
        # the quantity measured is milliseconds and the clock is not the limit.
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|i[3-6]86)$")
            add_subdirectory(benchmark/routing)
        else()
            message(STATUS
                "route_bench skipped: needs rdtsc, and CMAKE_SYSTEM_PROCESSOR is "
                "'${CMAKE_SYSTEM_PROCESSOR}'. The end-to-end routing level still builds.")
        endif()
    endif()
endif()

# Fuzz harnesses. Clang only, since libFuzzer has no MSVC or GCC equivalent, and off by
# default because turning it on instruments the library itself.
option(COROUTE_BUILD_FUZZERS "Build libFuzzer harnesses (Clang only)" OFF)
if(COROUTE_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()
