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
)

target_link_libraries(unit_tests PRIVATE coroute Catch2::Catch2WithMain)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
