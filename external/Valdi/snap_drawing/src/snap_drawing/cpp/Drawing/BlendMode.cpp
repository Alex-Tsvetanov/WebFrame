//
//  BlendMode.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/25/22.
//

#include "snap_drawing/cpp/Drawing/BlendMode.hpp"
#include "include/core/SkBlendMode.h"

namespace snap::drawing {

SkBlendMode toSkBlendMode(BlendMode blendMode) {
    switch (blendMode) {
        case BlendModeClear:
            return SkBlendMode::kClear;
        case BlendModeSrc:
            return SkBlendMode::kSrc;
        case BlendModeDst:
            return SkBlendMode::kDst;
        case BlendModeSrcOver:
            return SkBlendMode::kSrcOver;
        case BlendModeDstOver:
            return SkBlendMode::kDstOver;
        case BlendModeSrcIn:
            return SkBlendMode::kSrcIn;
        case BlendModeDstIn:
            return SkBlendMode::kDstIn;
        case BlendModeSrcOut:
            return SkBlendMode::kSrcOut;
        case BlendModeDstOut:
            return SkBlendMode::kDstOut;
        case BlendModeSrcATop:
            return SkBlendMode::kSrcATop;
        case BlendModeDstATop:
            return SkBlendMode::kDstATop;
        case BlendModeXor:
            return SkBlendMode::kXor;
        case BlendModePlus:
            return SkBlendMode::kPlus;
        case BlendModeModulate:
            return SkBlendMode::kModulate;
        case BlendModeScreen:
            return SkBlendMode::kScreen;
        case BlendModeLastCoeffMode:
            return SkBlendMode::kLastCoeffMode;
        case BlendModeOverlay:
            return SkBlendMode::kOverlay;
        case BlendModeDarken:
            return SkBlendMode::kDarken;
        case BlendModeLighten:
            return SkBlendMode::kLighten;
        case BlendModeColorDodge:
            return SkBlendMode::kColorDodge;
        case BlendModeColorBurn:
            return SkBlendMode::kColorBurn;
        case BlendModeHardLight:
            return SkBlendMode::kHardLight;
        case BlendModeSoftLight:
            return SkBlendMode::kSoftLight;
        case BlendModeDifference:
            return SkBlendMode::kDifference;
        case BlendModeExclusion:
            return SkBlendMode::kExclusion;
        case BlendModeMultiply:
            return SkBlendMode::kMultiply;
        case BlendModeLastSeparableMode:
            return SkBlendMode::kLastSeparableMode;
        case BlendModeHue:
            return SkBlendMode::kHue;
        case BlendModeSaturation:
            return SkBlendMode::kSaturation;
        case BlendModeColor:
            return SkBlendMode::kColor;
        case BlendModeLuminosity:
            return SkBlendMode::kLuminosity;
    }
}

} // namespace snap::drawing
