#pragma once

#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <vector>

using namespace Valdi;

std::vector<StringBox> internStrings(StringCache& stringCache, const std::vector<std::string>& strings);
std::string makeRandomString(size_t strLength);
std::vector<std::string> makeRandomStrings(size_t strLength);
std::vector<StringBox> makeRandomInternedStrings(StringCache& stringCache, size_t numberOfStrings, size_t strLength);
