#include "utils/debugging/detail/AssertInternals.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>

namespace snap::detail {

std::string combineString(std::string_view msg, const char* expressionStr) {
    if (!msg.empty()) {
        std::string_view expr(expressionStr);
        std::string out;
        out.reserve(msg.length() + 2 + expr.length());
        out += msg;
        out += ": ";
        out += expr;

        return out;
    }

    return expressionStr;
}

} // namespace snap::detail
