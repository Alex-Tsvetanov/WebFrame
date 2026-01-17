//
//  BridgeLayer.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/20/20.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Layers/ExternalLayer.hpp"

#include "valdi/snap_drawing/BridgedView.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"

#include <cstdint>

namespace Valdi {
class IBitmap;
class IViewManager;
class ViewFactory;
class ILogger;
} // namespace Valdi

namespace snap::drawing {

class BridgeGestureRecognizer;

class BridgeLayer : public ExternalLayer {
public:
    explicit BridgeLayer(const Ref<Resources>& resources);
    ~BridgeLayer() override;

    void onInitialize() override;

    bool prepareForReuse() override;

    Valdi::Ref<Valdi::View> getView() const;
    void setBridgedView(const Valdi::Ref<BridgedView>& bridgedView);
    Valdi::Ref<BridgedView> getBridgedView() const;

    void setAttachedData(const Ref<Valdi::RefCountable>& attachedData) override;

    bool hitTest(const Point& point) const override;

protected:
    void onLayout() override;

private:
    friend BridgeGestureRecognizer;

    bool forwardTouchEvent(const TouchEvent& currentEvent, TouchEventType type, bool isTouchOwner) const;
};

} // namespace snap::drawing
