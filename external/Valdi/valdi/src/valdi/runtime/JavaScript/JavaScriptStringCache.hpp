//
//  JavaScriptStringCache.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/31/19.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <optional>
#include <vector>

namespace Valdi {

class JavaScriptStringCache {
public:
    size_t store(StringBox str);

    std::optional<StringBox> get(size_t id) const;

private:
    FlatMap<StringBox, size_t> _indexByString;
    std::vector<StringBox> _strings;
};

} // namespace Valdi
