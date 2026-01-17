#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <vector>

namespace Valdi {

class LocaleResolver {
public:
    static std::vector<StringBox> getPreferredLocales();
};

} // namespace Valdi
