//
//  IFrameScheduler.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace snap::drawing {

class IFrameCallback : public Valdi::SimpleRefCountable {
public:
    virtual void onFrame(TimePoint time) = 0;
};

class IFrameScheduler : public Valdi::SimpleRefCountable {
public:
    /**
     Schedule a function to be executed on the next VSYNC of the display.
     */
    virtual void onNextVSync(const Ref<IFrameCallback>& callback) = 0;

    virtual void onMainThread(const Ref<IFrameCallback>& callback) = 0;
};

} // namespace snap::drawing
