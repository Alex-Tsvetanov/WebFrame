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
# Trapping UBSan alongside ASan. The trapping form rather than -fsanitize=undefined
# because the UBSan runtime is thin on the Windows target, and a trap is all a
# fuzzer needs: libFuzzer reports it as a crash either way. Signed overflow is the
# one that matters here, since a length computed by overflowing is exactly the
# shape of a smuggling primitive, and ASan alone does not see it.
set(COROUTE_FUZZ_UBSAN -fsanitize=signed-integer-overflow,shift,integer-divide-by-zero
                       -fsanitize-trap=undefined)

target_compile_options(coroute PRIVATE -fsanitize=fuzzer-no-link,address ${COROUTE_FUZZ_UBSAN}
                                       -fno-omit-frame-pointer)
target_link_options(coroute PUBLIC -fsanitize=address)

function(coroute_add_fuzzer name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE coroute)
    target_compile_options(${name} PRIVATE -fsanitize=fuzzer,address ${COROUTE_FUZZ_UBSAN}
                                          -fno-omit-frame-pointer)
    target_link_options(${name} PRIVATE -fsanitize=fuzzer,address)
endfunction()

coroute_add_fuzzer(fuzz_h2_frame)
coroute_add_fuzzer(fuzz_range)
coroute_add_fuzzer(fuzz_urlencoded)
coroute_add_fuzzer(fuzz_multipart)
coroute_add_fuzzer(fuzz_chunk_size)
coroute_add_fuzzer(fuzz_cookie)

# HPACK needs nghttp2, so it only exists when HTTP/2 is compiled in.
if(COROUTE_ENABLE_HTTP2)
    coroute_add_fuzzer(fuzz_hpack)
endif()

# corpus/<name>/ holds inputs libFuzzer found interesting, committed so a run starts
# from what previous runs learned instead of from nothing. Pass the directory as the
# first argument and new finds are added to it:
#
#   fuzz_range ../../../fuzz/corpus/range -max_total_time=300
#
# A crash artifact is written to the working directory, not the corpus.
