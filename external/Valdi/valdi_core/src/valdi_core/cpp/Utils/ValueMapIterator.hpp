//
//  ValueMapIterator.hpp
//  valdi-android
//
//  Created by Simon Corsin on 7/25/19.
//

#pragma once

#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"
#include <optional>

namespace Valdi {

class ValueMapIterator : public ValdiObject {
public:
    explicit ValueMapIterator(Ref<ValueMap> map);
    ~ValueMapIterator() override;

    std::optional<std::pair<StringBox, Value>> next();

    VALDI_CLASS_HEADER(ValueMapIterator);

private:
    Ref<ValueMap> _map;
    ValueMap::iterator _it;
};

} // namespace Valdi
