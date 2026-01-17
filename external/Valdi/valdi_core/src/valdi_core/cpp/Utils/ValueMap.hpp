//
//  ValueMap.hpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 5/5/21.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class ValueMap : public SimpleRefCountable, public FlatMap<StringBox, Value> {
public:
    ~ValueMap() override;
};

} // namespace Valdi
