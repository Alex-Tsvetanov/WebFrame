//
//  ValueMapIterator.cpp
//  valdi-android
//
//  Created by Simon Corsin on 7/25/19.
//

#include "valdi_core/cpp/Utils/ValueMapIterator.hpp"

namespace Valdi {

ValueMapIterator::ValueMapIterator(Ref<ValueMap> map) : _map(std::move(map)) {
    _it = _map->begin();
}

ValueMapIterator::~ValueMapIterator() = default;

std::optional<std::pair<StringBox, Value>> ValueMapIterator::next() {
    if (_it == _map->end()) {
        return {};
    }

    auto pair = std::make_pair(_it->first, _it->second);
    _it++;

    return {std::move(pair)};
}

VALDI_CLASS_IMPL(ValueMapIterator)

} // namespace Valdi
