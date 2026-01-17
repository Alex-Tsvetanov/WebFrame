// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

using AttributionFunctionCallback = DispatchFunction;

using AttributionFunction = const void* (*)(const AttributionFunctionCallback&);

struct Attribution {
    StringBox moduleName;
    StringBox owner;
    AttributionFunction fn;

    Attribution(const StringBox& moduleName, const StringBox& owner, AttributionFunction fn);
    ~Attribution();
};

} // namespace Valdi
