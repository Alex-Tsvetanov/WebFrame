//
//  CVDisplayLinkFrameScheduler.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#include "utils/platform/TargetPlatform.hpp"

#if __APPLE__ && !SC_IOS
#include "snap_drawing/apple/Drawing/CVDisplayLinkFrameScheduler.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

extern "C" {
void* objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void* pool);
}

namespace snap::drawing {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

CVDisplayLinkFrameScheduler::CVDisplayLinkFrameScheduler(Valdi::ILogger& logger)
    : BaseDisplayLinkFrameScheduler(logger) {}

CVDisplayLinkFrameScheduler::~CVDisplayLinkFrameScheduler() {
    auto guard = lock();
    destroyDisplayLink();
}

void CVDisplayLinkFrameScheduler::setActiveDisplay(CGDirectDisplayID displayId) {
    auto guard = lock();

    if (displayId != _activeDisplay) {
        destroyDisplayLink();
        _activeDisplay = displayId;
        createDisplayLink(guard);
        onDisplayLinkChanged(guard);
    }
}

void CVDisplayLinkFrameScheduler::destroyDisplayLink() {
    if (_activeDisplay != kCGNullDirectDisplay) {
        if (_displayLink) {
            CVDisplayLinkStop(_displayLink);
            CVDisplayLinkRelease(_displayLink);
            _displayLink = nil;
        }

        _activeDisplay = kCGNullDirectDisplay;
    }
}

void CVDisplayLinkFrameScheduler::createDisplayLink(std::unique_lock<Valdi::Mutex>& lock) {
    if (_activeDisplay == kCGNullDirectDisplay) {
        return;
    }

    auto code = CVDisplayLinkCreateWithCGDisplay(_activeDisplay, &_displayLink);
    if (code != kCVReturnSuccess) {
        VALDI_ERROR(getLogger(), "Failed to create DisplayLink: {}", code);
        return;
    }

    CVDisplayLinkSetOutputCallback(_displayLink, &CVDisplayLinkFrameScheduler::displayLinkCallback, this);
}

void CVDisplayLinkFrameScheduler::onResume(std::unique_lock<Valdi::Mutex>& lock) {
    auto displayLink = _displayLink;

    if (displayLink) {
        lock.unlock();
        CVDisplayLinkStart(displayLink);
    }
}

void CVDisplayLinkFrameScheduler::onPause(std::unique_lock<Valdi::Mutex>& lock) {
    auto displayLink = _displayLink;

    if (displayLink) {
        lock.unlock();
        CVDisplayLinkStop(displayLink);
    }
}

CVReturn CVDisplayLinkFrameScheduler::displayLinkCallback(CVDisplayLinkRef displayLink,
                                                          const CVTimeStamp* inNow,
                                                          const CVTimeStamp* inOutputTime,
                                                          CVOptionFlags flagsIn,
                                                          CVOptionFlags* flagsOut,
                                                          void* displayLinkContext) {
    auto pool = objc_autoreleasePoolPush();

    auto frameScheduler = Valdi::strongSmallRef(reinterpret_cast<CVDisplayLinkFrameScheduler*>(displayLinkContext));

    frameScheduler->onVSync();

    objc_autoreleasePoolPop(pool);

    return kCVReturnSuccess;
}

#pragma clang diagnostic pop

} // namespace snap::drawing

#endif
