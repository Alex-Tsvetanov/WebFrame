#include "utils/platform/BuildOptions.hpp"
#include <gtest/gtest.h>

namespace {

TEST(Dummy, canRunTest) {
    ASSERT_TRUE(snap::kAssertsCompiledIn);
    ASSERT_TRUE(snap::kLoggingCompiledIn);
    ASSERT_TRUE(snap::kTracingEnabled);
}

} // namespace
