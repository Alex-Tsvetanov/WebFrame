//
//  BlendMode.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/25/22.
//

#pragma once

#include "include/core/SkBlendMode.h"

namespace snap::drawing {

enum BlendMode {
    BlendModeClear,             //!< replaces destination with zero: fully transparent
    BlendModeSrc,               //!< replaces destination
    BlendModeDst,               //!< preserves destination
    BlendModeSrcOver,           //!< source over destination
    BlendModeDstOver,           //!< destination over source
    BlendModeSrcIn,             //!< source trimmed inside destination
    BlendModeDstIn,             //!< destination trimmed by source
    BlendModeSrcOut,            //!< source trimmed outside destination
    BlendModeDstOut,            //!< destination trimmed outside source
    BlendModeSrcATop,           //!< source inside destination blended with destination
    BlendModeDstATop,           //!< destination inside source blended with source
    BlendModeXor,               //!< each of source and destination trimmed outside the other
    BlendModePlus,              //!< sum of colors
    BlendModeModulate,          //!< product of premultiplied colors; darkens destination
    BlendModeScreen,            //!< multiply inverse of pixels, inverting result; brightens destination
    BlendModeLastCoeffMode,     //!< last porter duff blend mode
    BlendModeOverlay,           //!< multiply or screen, depending on destination
    BlendModeDarken,            //!< darker of source and destination
    BlendModeLighten,           //!< lighter of source and destination
    BlendModeColorDodge,        //!< brighten destination to reflect source
    BlendModeColorBurn,         //!< darken destination to reflect source
    BlendModeHardLight,         //!< multiply or screen, depending on source
    BlendModeSoftLight,         //!< lighten or darken, depending on source
    BlendModeDifference,        //!< subtract darker from lighter with higher contrast
    BlendModeExclusion,         //!< subtract darker from lighter with lower contrast
    BlendModeMultiply,          //!< multiply source with destination, darkening image
    BlendModeLastSeparableMode, //!< last blend mode operating separately on components
    BlendModeHue,               //!< hue of source with saturation and luminosity of destination
    BlendModeSaturation,        //!< saturation of source with hue and luminosity of destination
    BlendModeColor,             //!< hue and saturation of source with luminosity of destination
    BlendModeLuminosity,        //!< luminosity of source with hue and saturation of destination
};

SkBlendMode toSkBlendMode(BlendMode blendMode);

} // namespace snap::drawing
