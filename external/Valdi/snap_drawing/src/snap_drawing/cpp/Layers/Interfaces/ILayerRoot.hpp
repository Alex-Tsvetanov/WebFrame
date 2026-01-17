//
//  ILayerRoot.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/19/22.
//

#pragma once

#include "snap_drawing/cpp/Events/EventCallback.hpp"
#include "snap_drawing/cpp/Events/EventId.hpp"
#include "snap_drawing/cpp/Layers/Interfaces/ILayer.hpp"
#include "snap_drawing/cpp/Utils/Duration.hpp"

namespace snap::drawing {

class ILayerRoot : public ILayer {
public:
    virtual EventId enqueueEvent(EventCallback&& eventCallback, Duration after) = 0;
    virtual bool cancelEvent(EventId eventId) = 0;

    virtual LayerId allocateLayerId() = 0;

    Ref<ILayer> getParent() const final {
        return nullptr;
    }

    virtual bool shouldRasterizeExternalSurface() const = 0;
};

} // namespace snap::drawing
