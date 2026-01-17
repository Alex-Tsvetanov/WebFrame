//
//  SCValdiCursorUpdater.m
//  valdi-macos
//
//  Created by Simon Corsin on 10/29/21.
//

#import "SCValdiCursorUpdater.h"

@implementation SCValdiCursorUpdater {
    NSTrackingArea *_trackingArea;
    __weak NSView *_view;
    SCValdiCursorType _cursorType;
}

- (instancetype)initWithView:(NSView *)view
{
    self = [super init];

    if (self) {
        _view = view;
        _cursorType = SCValdiCursorTypePointer;
    }

    return self;
}

- (void)dealloc
{
    [self _removeTrackingArea];
}

- (void)_removeTrackingArea
{
    if (_trackingArea) {
        [_view removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }
}

- (void)setTrackingAreaSize:(CGSize)trackingAreaSize
{
    if (!CGSizeEqualToSize(_trackingAreaSize, trackingAreaSize)) {
        _trackingAreaSize = trackingAreaSize;

        [self _removeTrackingArea];

        if (!CGSizeEqualToSize(trackingAreaSize, CGSizeZero)) {
            _trackingArea = [[NSTrackingArea alloc] initWithRect:NSMakeRect(0, 0, trackingAreaSize.width, trackingAreaSize.height)
                                                         options:NSTrackingActiveInActiveApp | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved
                                                           owner:self
                                                        userInfo:nil];
            [_view addTrackingArea:_trackingArea];
        }
    }
}

- (void)_updateCursorType:(SCValdiCursorType)cursorType
{
    if (_cursorType == cursorType) {
        return;
    }
    _cursorType = cursorType;
    switch (cursorType) {
        case SCValdiCursorTypePointer:
            [[NSCursor arrowCursor] set];
            break;
        case SCValdiCursorTypeHand:
            [[NSCursor pointingHandCursor] set];
            break;
        case SCValdiCursorTypeOpenHand:
            [[NSCursor openHandCursor] set];
            break;
    }
}

- (void)_updateCursorWithEvent:(NSEvent *)event
{
    SCValdiCursorType cursorType = [self.delegate cursorUpdater:self cursorTypeAtPoint:event.locationInWindow];
    [self _updateCursorType:cursorType];
}

- (void)mouseEntered:(NSEvent *)event
{
    [self _updateCursorWithEvent:event];
}

- (void)mouseExited:(NSEvent *)event
{
    [self _updateCursorType:SCValdiCursorTypePointer];
}

- (void)mouseMoved:(NSEvent *)event
{
    [self _updateCursorWithEvent:event];
}

@end
