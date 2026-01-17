//
//  MapValdiObject.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/11/19.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

template<typename T>
class MapValdiObject : public ValdiObject, public FlatMap<StringBox, T> {
public:
    VALDI_CLASS_HEADER_IMPL(MapValdiObject)
};

template<typename T>
using MapValdiObjectPtr = Ref<MapValdiObject<T>>;

} // namespace Valdi
