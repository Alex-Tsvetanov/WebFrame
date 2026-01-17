//
//  LazyValueConvertible.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 1/8/20.
//

#include "valdi_core/cpp/Utils/LazyValueConvertible.hpp"

namespace Valdi {

LazyValueConvertible::LazyValueConvertible(Function<Value()> factory) : Lazy(std::move(factory)) {}

Value LazyValueConvertible::toValue() {
    return getOrCreate();
}

Shared<LazyValueConvertible> LazyValueConvertible::fromValue(const Value& value) {
    return makeShared<Valdi::LazyValueConvertible>([value]() { return value; });
}

} // namespace Valdi
