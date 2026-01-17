//
//  Resources.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/12/22.
//

#include "snap_drawing/cpp/Resources.hpp"
#include "include/core/SkGraphics.h"
#include "snap_drawing/cpp/Utils/Image.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"

namespace snap::drawing {

Resources::Resources(const Ref<FontManager>& fontManager, Scalar displayScale, Valdi::ILogger& logger)
    : Resources(fontManager, displayScale, GesturesConfiguration::getDefault(), logger) {}

Resources::Resources(const Ref<FontManager>& fontManager,
                     Scalar displayScale,
                     const GesturesConfiguration& gesturesConfiguration,
                     Valdi::ILogger& logger)
    : _fontManager(fontManager),
      _respectDynamicType(true),
      _displayScale(displayScale),
      _dynamicTypeScale(1.0),
      _gesturesConfiguration(gesturesConfiguration),
      _logger(&logger) {
    // Make sure all Skia features are properly loaded.
    // This will be a no-op if this call was already done before.
    SkGraphics::Init();
    Image::initializeCodecs();
}

Resources::~Resources() = default;

const Ref<FontManager>& Resources::getFontManager() const {
    return _fontManager;
}

bool Resources::getRespectDynamicType() const {
    return _respectDynamicType;
}

void Resources::setRespectDynamicType(bool respectDynamicType) {
    _respectDynamicType = respectDynamicType;
}

Scalar Resources::getDisplayScale() const {
    return _displayScale;
}

void Resources::setDisplayScale(Scalar displayScale) {
    _displayScale = displayScale;
}

Scalar Resources::getDynamicTypeScale() const {
    return _dynamicTypeScale;
}

void Resources::setDynamicTypeScale(Scalar dynamicTypeScale) {
    _dynamicTypeScale = dynamicTypeScale;
}

const GesturesConfiguration& Resources::getGesturesConfiguration() const {
    return _gesturesConfiguration;
}

Valdi::ILogger& Resources::getLogger() const {
    return *_logger;
}

} // namespace snap::drawing
