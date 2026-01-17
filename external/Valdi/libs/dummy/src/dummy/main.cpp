#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "utils/base/Function.hpp"
#include "utils/base/NonCopyable.hpp"
#include "utils/debugging/Assert.hpp"
#include "utils/platform/TargetPlatform.hpp"

#include "hermes/VM/Runtime.h"

#include <fmt/format.h>
#include <json/reader.h>
#include <phmap/phmap.h>

static std::string getPlatformString() {
    switch (snap::kPlatform) {
        case snap::Platform::Android:
            return "android";
        case snap::Platform::Ios:
            return "ios";
        case snap::Platform::MacOs:
            return "macos";
        case snap::Platform::DesktopLinux:
            return "linux";
        case snap::Platform::Wasm:
            return "macos";
    }
}

int main(int argc, const char** argv) {
    SC_ASSERT(true);

    std::ostringstream ss;
    ss << getPlatformString() << " [";

    std::vector<std::string> options;
    if constexpr (snap::kAssertsCompiledIn) {
        options.emplace_back("asserts");
    }
    if constexpr (snap::kLoggingCompiledIn) {
        options.emplace_back("logging");
    }
    if constexpr (snap::kTracingEnabled) {
        options.emplace_back("tracing");
    }

    std::copy(options.begin(), options.end(), std::ostream_iterator<std::string>(ss, ","));

    ss << "]";

    std::cout << ss.str() << std::endl;

    return 0;
}
