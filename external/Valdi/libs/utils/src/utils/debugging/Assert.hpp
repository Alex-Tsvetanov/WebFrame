#pragma once

// This header adds assertion macros that can be used to validate conditions.
//
// SC_ASSERT(expression, "optional message here");
// SC_ASSERT(expression);
//
// Asserts are enabled in all internal builds (Debug, Master, Alpha) and disabled for all
// AppStore builds (Beta and Production).
//
// Fused Asserts:
//    In non-debug builds (Master and Alpha) asserts may be fusible which
//    helps to prevent crash loops.  Once an assertion is hit and the app aborts,
//    future assertions will be disabled on that line for 30 minutes.
//
// Aborts will crash the app for all builds, including AppStore builds.  Be extremely
// careful with their usage and use them only when the app is in an unrecoverable state.

#include "utils/debugging/detail/AssertInternals.hpp"
#include "utils/platform/BuildOptions.hpp"

#include <cstdlib>
#include <string>
#include <string_view>

// Aborts the application when releaseAssertsEnabled is true and the expression
// evaluates to false.
//
// Usage:
//   SC_ASSERT(expression)
//   SC_ASSERT(expression, message)
#define SC_ASSERT(...) _SC_PICK_ASSERTION_ARGS(__VA_ARGS__, _SC_WITH_MESSAGE, _SC_WITHOUT_MESSAGE)(__VA_ARGS__)

// Prints the passed message and crashes the process when releaseAssertsEnabled is true.
// Same as SC_ASSERT(false, message)
//
// Usage:
//   SC_ASSERT_FAIL(message)
#define SC_ASSERT_FAIL(message)                                                                                        \
    do {                                                                                                               \
        if constexpr (::snap::kAssertsCompiledIn) {                                                                    \
            if (::snap::releaseAssertsEnabled()) {                                                                     \
                _SC_SYSTEM_ASSERT_MAYBE_FUSED(__FILE__, __LINE__, ::snap::detail::messageToCString(message));          \
            }                                                                                                          \
        }                                                                                                              \
    } while (false)

// Shortcut for nullptr check.
//
// This macro works will all pointer types including std::unique_ptr and std::shared_ptr.
//
// Usage:
//   std::shared_ptr<T> myPtr = getSharedT();
//   SC_ASSERT_NOTNULL(myPtr);
#define SC_ASSERT_NOTNULL(expr) SC_ASSERT((expr) != nullptr)

// Production assertion macro.
//
// ** BE EXTREMELY CAREFUL AND READ THIS DOCUMENTATION BEFORE USING **
//
// This will always abort the app when the condition evalutes to false, even in release builds.
// Only use this for fatal, unrecoverable states that require us to crash at the site where the
// unrecoverable state is found.
//
// For example, programming and logic errors which would leave the app in a broken and/or invalid
// state.  In general, do not use this unless you are ready to treat the occurrence of this in a
// release build as UBN.
//
// WARNING: The stringified expression and assert message will be embedded into the binary. Make
//          sure you don't leak any private info!
//
// Usage:
//   SC_ABORT(message)
//   SC_ABORT_UNLESS(expression, message)
#define SC_ABORT_UNLESS(expr, msg)                                                                                     \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            if constexpr (::snap::kAssertsCompiledIn) {                                                                \
                _SC_CALL_SYSTEM_ASSERT(msg, #expr, __FILE__, __LINE__);                                                \
            } else {                                                                                                   \
                _SC_CALL_SYSTEM_ASSERT(msg, #expr, "unknown", __LINE__);                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (false)

#define SC_ABORT(msg) SC_ABORT_UNLESS(false, msg)

namespace snap {

// Returns true if SC_ASSERT is enabled. Used for disabling asserts in runtime.
bool releaseAssertsEnabled();

// Enables/disables SC_ASSERT
void setEnableReleaseAsserts(bool enable);

// Fuse the assertion. Returns true if assertion is fused by this call. Returns
// false if assertion was already fused. In any case, when this call returns,
// the assertion is in fused state.
bool fuseAssertion(std::string_view key);

// == FOR TESTING ONLY==
// Clear all fused assertions. Next SC_ASSERT will trigger.
void resetFusedAssertions();
// == FOR TESTING ONLY==
// Re-read fused assertions from persist storage.
void reloadFusedAssertions();

// Returns true if fused assertions are enabled
bool fusedAssertsEnabled();

// Enables/disables fused assertions
void setEnableFusedAsserts(bool enable);

} // namespace snap
