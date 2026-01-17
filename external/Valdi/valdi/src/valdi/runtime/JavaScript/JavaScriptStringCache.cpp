//
//  JavaScriptStringCache.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/31/19.
//

#include "valdi/runtime/JavaScript/JavaScriptStringCache.hpp"

namespace Valdi {

size_t JavaScriptStringCache::store(StringBox str) {
    const auto& it = _indexByString.find(str);
    if (it != _indexByString.end()) {
        return it->second;
    }

    auto id = _strings.size();
    _indexByString[str] = id;
    _strings.emplace_back(std::move(str));
    return id;
}

std::optional<StringBox> JavaScriptStringCache::get(size_t id) const {
    if (id >= _strings.size()) {
        return std::nullopt;
    }

    return {_strings[id]};
}

} // namespace Valdi
