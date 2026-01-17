//
//  SimpleAtomicCancelable.cpp
//  valdi_core-pc
//
//  Created by Ramzy Jaber on 3/16/22.
//

#include "valdi_core/cpp/Utils/SimpleAtomicCancelable.hpp"

namespace Valdi {
SimpleAtomicCancelable::SimpleAtomicCancelable() : _wasCanceled(false) {};

SimpleAtomicCancelable::~SimpleAtomicCancelable() = default;

bool SimpleAtomicCancelable::SimpleAtomicCancelable::wasCanceled() {
    return _wasCanceled.load();
}

void SimpleAtomicCancelable::cancel() {
    _wasCanceled.exchange(true);
}

} // namespace Valdi
