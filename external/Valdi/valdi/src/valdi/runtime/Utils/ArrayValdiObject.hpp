//
//  ArrayValdiObject.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/11/19.
//

#pragma once

#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include <vector>

namespace Valdi {

template<typename T>
class ArrayValdiObject : public ValdiObject, public std::vector<T> {
public:
    VALDI_CLASS_HEADER_IMPL(ArrayValdiObject)
};

template<typename T>
using ArrayValdiObjectPtr = Ref<ArrayValdiObject<T>>;

} // namespace Valdi
