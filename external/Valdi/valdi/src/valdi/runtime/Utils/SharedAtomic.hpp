//
//  AtomicValue.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 3/20/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Holder.hpp"

namespace Valdi {

/**
 An object which can store and fetch a value under a mutex.
 The SharedAtomic object can be copied and shared across threads.
 */
template<typename T>
using SharedAtomic = Holder<T>;

template<typename T>
using SharedAtomicObject = SharedAtomic<Ref<T>>;

} // namespace Valdi
