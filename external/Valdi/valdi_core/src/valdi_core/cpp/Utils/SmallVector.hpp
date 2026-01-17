//
//  SmallVector.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/31/19.
//

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-bool-conversion"

#include <boost/container/small_vector.hpp>

#pragma clang diagnostic pop

namespace Valdi {

template<typename T, std::size_t size>
using SmallVector = boost::container::small_vector<T, size>;

template<typename T>
using SmallVectorBase = boost::container::small_vector_base<T>;

} // namespace Valdi
