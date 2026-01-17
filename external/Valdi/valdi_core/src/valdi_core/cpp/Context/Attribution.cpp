// Copyright © 2023 Snap, Inc. All rights reserved.

#include "valdi_core/cpp/Context/Attribution.hpp"

namespace Valdi {

Attribution::Attribution(const StringBox& moduleName, const StringBox& owner, AttributionFunction fn)
    : moduleName(moduleName), owner(owner), fn(fn) {}
Attribution::~Attribution() = default;

} // namespace Valdi
