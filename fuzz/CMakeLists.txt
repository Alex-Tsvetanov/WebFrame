# Fuzz harnesses.
#
# Off by default and Clang only: -fsanitize=fuzzer is libFuzzer, which ships with
# Clang's runtime and has no MSVC or GCC equivalent. Configure with
#
#   cmake --preset fuzz && cmake --build build/fuzz
#
# Each target is a standalone executable. Run one with a corpus directory and a time
# budget, for example:
#
#   build/fuzz/fuzz/fuzz_h2_frame corpus/h2_frame -max_total_time=300
#
# On Windows the ASan runtime is a DLL that is not on PATH by default. Add
# <LLVM>/lib/clang/<major>/lib/windows to PATH for the run, or the executable exits
# with STATUS_DLL_NOT_FOUND and no message at all.
#
# What belongs here: pure functions that take bytes from an unauthenticated peer.
# Anything needing a Connection or an event loop does not fuzz well and belongs in a
# conformance suite instead.

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "COROUTE_BUILD_FUZZERS requires Clang: libFuzzer has no MSVC or GCC equivalent")
endif()

# fuzzer-no-link on the library so the code under test is instrumented for coverage
# guidance, while only the harnesses link the libFuzzer main.
target_compile_options(coroute PRIVATE -fsanitize=fuzzer-no-link,address -fno-omit-frame-pointer)
target_link_options(coroute PUBLIC -fsanitize=address)

function(coroute_add_fuzzer name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE coroute)
    target_compile_options(${name} PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
    target_link_options(${name} PRIVATE -fsanitize=fuzzer,address)
endfunction()

coroute_add_fuzzer(fuzz_h2_frame)
coroute_add_fuzzer(fuzz_range)
