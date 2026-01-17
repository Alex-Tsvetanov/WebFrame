//
//  CVDisplayLinkFrameScheduler.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#pragma once

#include "snap_drawing/apple/Drawing/BaseDisplayLinkFrameScheduler.hpp"

#include <QuartzCore/CADisplayLink.h>

@class SCSnapDrawingDisplayLinkTarget;

namespace snap::drawing {

using CallbackQueue = Valdi::SmallVector<Ref<IFrameCallback>, 2>;

class CADisplayLinkFrameScheduler : public BaseDisplayLinkFrameScheduler {
public:
    explicit CADisplayLinkFrameScheduler(Valdi::ILogger& logger);
    ~CADisplayLinkFrameScheduler() override;

protected:
    void onResume(std::unique_lock<Valdi::Mutex>& lock) override;
    void onPause(std::unique_lock<Valdi::Mutex>& lock) override;

private:
    CADisplayLink* _displayLink;
};

} // namespace snap::drawing
