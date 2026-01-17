#include "utils/debugging/detail/AssertInternals.hpp"

namespace snap {

namespace detail {
void reportNonFatalAssert(std::string_view path, int64_t line, std::string_view message) {
    // no-op
}
} // namespace detail

bool fuseAssertion(std::string_view key) {
    // no-op
    return true;
}

} // namespace snap
