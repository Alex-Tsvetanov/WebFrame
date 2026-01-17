//
//  ILayer.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 6/28/20.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace snap::drawing {

using LayerId = uint64_t;
constexpr LayerId kLayerIdNone = 0;

class ILayer : public Valdi::SharedPtrRefCountable {
public:
    /**
     Will be called right after the constructor when using the makeLayer factory function.
     */
    virtual void onInitialize() = 0;
    virtual void setChildNeedsDisplay() = 0;
    virtual void requestLayout(ILayer* layer) = 0;
    virtual void requestFocus(ILayer* layer) = 0;
    virtual Ref<ILayer> getParent() const = 0;
};

} // namespace snap::drawing
