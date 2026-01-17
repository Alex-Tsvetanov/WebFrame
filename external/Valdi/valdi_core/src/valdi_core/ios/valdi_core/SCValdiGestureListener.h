//
//  SCValdiGestureListener.h
//  valdi_core-ios
//
//  Created by Simon Corsin on 12/14/22.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, SCValdiGestureType) {
    SCValdiGestureTypeTap,
    SCValdiGestureTypeDoubleTap,
    SCValdiGestureTypeLongPress,
    SCValdiGestureTypeDrag,
    SCValdiGestureTypePinch,
    SCValdiGestureTypeRotate,
    SCValdiGestureTypeTouch,
};

/**
 SCValdiGestureListener is a protocol that can be used to be notified when a Valdi
 gesture is being triggerd.
 */
@protocol SCValdiGestureListener <NSObject>

- (void)onGestureType:(SCValdiGestureType)gestureType
    didUpdateWithState:(UIGestureRecognizerState)state
                 event:(UIEvent*)event;

@end

NS_ASSUME_NONNULL_END
