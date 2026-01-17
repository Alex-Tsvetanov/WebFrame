//
//  ValueConvertible.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 9/30/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

/**
 An abstraction over some object which can be converted into a Value.
 */
class ValueConvertible {
public:
    virtual ~ValueConvertible() = default;

    virtual Value toValue() = 0;
};

} // namespace Valdi
