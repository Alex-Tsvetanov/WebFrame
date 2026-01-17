//
//  SCValdiLayerAnimation.m
//  Valdi
//
//  Created by Simon Corsin on 7/18/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi/ios/Animations/SCValdiLayerAnimation.h"

@interface SCValdiLayerAnimation () <CAAnimationDelegate>
@end

@implementation SCValdiLayerAnimation {
    void (^_completion)(BOOL);
    CAAnimation* _addedAnimationForKey;
    BOOL _disableRemoveOnComplete;
}

- (instancetype)initWithLayer:(CALayer *)layer animation:(CAAnimation *)animation key:(nonnull NSString *)key disableRemoveOnComplete:(BOOL)disableRemoveOnComplete
{
    self = [super init];
    if (self) {
        _layer = layer;
        _animation = animation;
        _key = key;
        _disableRemoveOnComplete = disableRemoveOnComplete;
    }
    return self;
}

- (void)performAnimationWithCompletion:(void (^)(BOOL))completion
{
    _animation.delegate = self;
    // There seems to be a bug in CAAnimation with setting removedOnCompletion to YES (default setting).
    // If the animation is attached at a layer that is not visible, the animationDidStop will run immediately with
    // finished = NO, but the animation will still be running. The key will also no longer be in the layer's animationKeys
    // so we are unable to stop the animation.
    // Setting removedOnCompletion to NO and removing the animation from the layer when it is done fixes this issue.
    if (_disableRemoveOnComplete) {
        _animation.removedOnCompletion = NO;
    } else {
        _animation.removedOnCompletion = YES;
    }

    if (_completion) {
        [_layer removeAnimationForKey:_key];
        // Check in case completion was called in animationDidStop: after removeAnimationForKey:
        if (_completion) {
            _completion(NO);
        }
    }

    _completion = completion;
    if (_layer.superlayer) {
        [_layer addAnimation:_animation forKey:_key];
        if (_disableRemoveOnComplete) {
            _addedAnimationForKey = [_layer animationForKey:_key];
        }
    } else {
        // If the layer is detached, we immediately cancel the animation as otherwise it could result in the
        // layer having the animation set as deferred internally within CoreAnimation with no way for us
        // to know at runtime that this is happening. "animationKeys" could return an empty array even though
        // the layer has the animation under the hood.
        [self animationDidStop:_animation finished:NO];
    }
}

- (void)cancelAnimationIfRunning
{
    // If the animation has completed or was clobbered, the completion will be nil.
    if (_completion) {
        [_layer removeAnimationForKey:_key];
    }
}

#pragma mark - CAAnimationDelegate

- (void)animationDidStop:(CAAnimation *)anim finished:(BOOL)flag
{
    // Remove the animation from the layer only if it's the same one we added.
    // If these don't match, this animation was overriden by another and has already been removed.

    if (_disableRemoveOnComplete) {
        if (_addedAnimationForKey && _addedAnimationForKey == [_layer animationForKey:_key]) {
            [_layer removeAnimationForKey:_key];
        }
    }

    void (^completion)(BOOL) = _completion;
    _completion = nil;
    _animation.delegate = nil;

    if (completion) {
        completion(flag);
    }
}

@end
