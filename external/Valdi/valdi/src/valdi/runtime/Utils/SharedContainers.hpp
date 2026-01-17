//
//  SharedContainers.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/10/19.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <vector>

namespace Valdi {

template<typename K, typename V>
using SharedMap = Shared<FlatMap<K, V>>;

template<typename V>
using SharedVector = Shared<std::vector<V>>;
} // namespace Valdi
