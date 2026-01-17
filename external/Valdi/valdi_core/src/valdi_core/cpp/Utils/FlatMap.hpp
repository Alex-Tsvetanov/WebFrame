//
//  FlatMap.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/23/19.
//

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wthread-safety-analysis"
#include <phmap/phmap.h>
#pragma clang diagnostic pop

namespace Valdi {

template<typename Key, typename Value>
using FlatMap = phmap::flat_hash_map<Key, Value>;

} // namespace Valdi
