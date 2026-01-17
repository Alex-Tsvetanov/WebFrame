#include "Stamp.hpp"

namespace Valdi {
std::string getToolboxVersion() {
    auto timestamp = BUILD_TIMESTAMP;

    return std::to_string(timestamp);
}
} // namespace Valdi
