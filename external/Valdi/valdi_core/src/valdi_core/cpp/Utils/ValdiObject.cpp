//
//  ValdiObject.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/10/19.
//

#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace Valdi {

ValdiObject::~ValdiObject() = default;

StringBox ValdiObject::getDebugId() const {
    return STRING_FORMAT("{}@{}", getClassName(), reinterpret_cast<const void*>(this));
}

StringBox ValdiObject::getDebugDescription() const {
    return getDebugId();
}

} // namespace Valdi
