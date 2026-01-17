//
//  LazyValueConvertible.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 1/8/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Lazy.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/ValueConvertible.hpp"

namespace Valdi {

/**
 A ValueConvertible which lazily calls a factory when required.
 */
class LazyValueConvertible : public Lazy<Value>, public ValueConvertible {
public:
    explicit LazyValueConvertible(Function<Value()> factory);

    Value toValue() override;

    static Shared<LazyValueConvertible> fromValue(const Value& value);
};

} // namespace Valdi
