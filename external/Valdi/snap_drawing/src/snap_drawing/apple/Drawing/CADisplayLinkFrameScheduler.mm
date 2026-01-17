//
//  CADisplayLinkFrameScheduler.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#include "utils/platform/TargetPlatform.hpp"

#if __APPLE__ && SC_IOS
#include "snap_drawing/apple/Drawing/CADisplayLinkFrameScheduler.h"
#import <Foundation/Foundation.h>

@interface SCSnapDrawingDisplayLinkTarget: NSObject

@end

@implementation SCSnapDrawingDisplayLinkTarget {
    snap::drawing::CADisplayLinkFrameScheduler *_frameScheduler;
}

- (instancetype)initWithScheduler:(snap::drawing::CADisplayLinkFrameScheduler *)frameScheduler
{
    self = [super init];

    if (self) {
        _frameScheduler = frameScheduler;
    }

    return self;
}

- (void)vsync
{
    _frameScheduler->onVSync();
}

@end

namespace snap::drawing {

CADisplayLinkFrameScheduler::CADisplayLinkFrameScheduler(Valdi::ILogger& logger) : BaseDisplayLinkFrameScheduler(logger) {
    SCSnapDrawingDisplayLinkTarget *target = [[SCSnapDrawingDisplayLinkTarget alloc] initWithScheduler:this];
    _displayLink = [CADisplayLink displayLinkWithTarget:target selector:@selector(vsync)];
}

CADisplayLinkFrameScheduler::~CADisplayLinkFrameScheduler() {
    [_displayLink invalidate];
    _displayLink = nil;
}

void CADisplayLinkFrameScheduler::onResume(std::unique_lock<Valdi::Mutex>& lock) {
    lock.unlock();
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

void CADisplayLinkFrameScheduler::onPause(std::unique_lock<Valdi::Mutex>& lock) {
    lock.unlock();
    [_displayLink removeFromRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

} // namespace snap::drawing

#endif
