//
//  Resources.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/12/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Scalar.hpp"

#include "snap_drawing/cpp/Text/FontManager.hpp"

namespace Valdi {
class ILogger;
}

namespace snap::drawing {

class Resources : public Valdi::SimpleRefCountable {
public:
    Resources(const Ref<FontManager>& fontManager,
              Scalar displayScale,
              const GesturesConfiguration& gesturesConfiguration,
              Valdi::ILogger& logger);

    Resources(const Ref<FontManager>& fontManager, Scalar displayScale, Valdi::ILogger& logger);

    ~Resources() override;

    const Ref<FontManager>& getFontManager() const;
    Valdi::ILogger& getLogger() const;

    bool getRespectDynamicType() const;
    void setRespectDynamicType(bool respectDynamicType);

    Scalar getDisplayScale() const;
    void setDisplayScale(Scalar displayScale);

    Scalar getDynamicTypeScale() const;
    void setDynamicTypeScale(Scalar dynamicTypeScale);

    const GesturesConfiguration& getGesturesConfiguration() const;

private:
    Ref<FontManager> _fontManager;
    bool _respectDynamicType;
    Scalar _displayScale;
    Scalar _dynamicTypeScale;
    GesturesConfiguration _gesturesConfiguration;
    Ref<Valdi::ILogger> _logger;
};

} // namespace snap::drawing
