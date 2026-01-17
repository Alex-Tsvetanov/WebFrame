//
//  FlatSet.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/7/19.
//

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wthread-safety-analysis"
#include <phmap/phmap.h>
#pragma clang diagnostic pop

namespace Valdi {

template<typename Key>
using FlatSet = phmap::flat_hash_set<Key>;

} // namespace Valdi
