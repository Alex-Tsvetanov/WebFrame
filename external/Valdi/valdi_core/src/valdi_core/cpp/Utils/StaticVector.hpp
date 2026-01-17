//
//  StaticVector.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/31/19.
//

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-bool-conversion"

#include <boost/container/static_vector.hpp>

#pragma clang diagnostic pop

namespace Valdi {

template<typename T, std::size_t size>
using StaticVector = boost::container::static_vector<T, size>;
}
