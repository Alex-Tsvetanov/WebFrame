#pragma once

namespace snap {

// Controls the way SC_ASSERT is embedded into the final binary. Asserts are compiled out if set to false.
constexpr bool kAssertsCompiledIn = true;

constexpr bool kLoggingCompiledIn = true;
// Macros version that can be used in #if expressions
#define SC_LOGGING_COMPILED_IN() true

// Controls tracing being compiled in. Not enabled if set to false.
constexpr bool kTracingEnabled = true;
#define SC_TRACING_COMPILED_IN() true

// If this build is a development build
constexpr bool kIsDevBuild = true;

constexpr bool kIsGoldBuild = false;

constexpr bool kIsAppstoreBuild = false;

constexpr bool kIsAlphaBuild = false;

} // namespace snap
