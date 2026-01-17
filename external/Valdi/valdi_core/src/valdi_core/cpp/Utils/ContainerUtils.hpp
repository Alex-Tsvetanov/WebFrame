//
//  ContainerUtils.hpp
//  utils-ios
//
//  Created by Simon Corsin on 8/13/18.
//

#pragma once

namespace Valdi {

template<typename ContainerT, typename PredicateT>
void eraseIf(ContainerT& items, const PredicateT&& predicate) {
    for (auto it = items.begin(); it != items.end();) {
        if (predicate(*it)) {
            it = items.erase(it);
        } else {
            ++it;
        }
    }
};

template<typename ContainerT, typename PredicateT>
bool eraseFirstIf(ContainerT& items, const PredicateT&& predicate) {
    for (auto it = items.begin(); it != items.end();) {
        if (predicate(*it)) {
            items.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
};

} // namespace Valdi
