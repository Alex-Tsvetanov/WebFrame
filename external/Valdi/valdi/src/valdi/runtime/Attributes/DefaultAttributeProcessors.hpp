//
//  DefaultAttributeProcessors.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 6/27/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Attributes/AttributesManager.hpp"

namespace Valdi {

Result<Value> preprocessBorder(const Ref<ColorPalette>& colorPalette, const Value& in);
Result<Value> preprocessBoxShadow(const Ref<ColorPalette>& colorPalette, const Value& in);
Result<Value> preprocessTextShadow(const Ref<ColorPalette>& colorPalette, const Value& in);
Result<Value> preprocessBorderRadius(const Ref<ColorPalette>& /*colorPalette*/, const Value& in);
Result<Value> preprocessGradient(const Ref<ColorPalette>& colorPalette, const Value& in);

Result<Value> postprocessBoxShadow(bool isRightToLeft, const Value& in);
Result<Value> postprocessBorderRadius(bool isRightToLeft, const Value& in);
Result<Value> postprocessGradient(bool isRightToLeft, const Value& in);

void registerDefaultProcessors(AttributesManager& attributesManager);

} // namespace Valdi
