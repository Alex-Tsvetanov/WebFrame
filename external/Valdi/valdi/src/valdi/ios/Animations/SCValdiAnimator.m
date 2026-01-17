
//
//  SCValdiAnimator.m
//  Valdi
//
//  Created by Simon Corsin on 7/16/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi/ios/Animations/SCValdiAnimator.h"

#import "valdi/ios/Animations/SCValdiLayerAnimation.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/UIView+ValdiBase.h"
#import "valdi_core/SCValdiFunction.h"

#import "valdi_core/SCValdiAction.h"

#if TARGET_IPHONE_SIMULATOR
#import <mach-o/dyld.h>
#import <dlfcn.h>
#import <objc/runtime.h>

static float (*UIAnimationDragCoefficient)(void) = NULL;
#endif

@implementation SCValdiAnimator {
    UIViewAnimationCurve _curve;
    NSArray<NSNumber *> *_controlPoints;
    NSTimeInterval _duration;
    BOOL _beginFromCurrentState;
    BOOL _crossfade;
    BOOL _wasCancelled;
    CGFloat _stiffness;
    CGFloat _damping;
    NSMutableArray<SCValdiLayerAnimation *> *_pendingLayerAnimations;
    NSMutableArray<SCValdiLayerAnimation *> *_runningLayerAnimations;
    dispatch_group_t _group;
    NSMutableArray *_completions;
    NSMutableDictionary<NSValue *, NSMutableArray<SCValdiLayerAnimation *> *> *_pendingAnimationsByLayer;
    BOOL _disableRemoveOnComplete;
}

#if TARGET_IPHONE_SIMULATOR
+ (void) load
{
    // https://gist.github.com/0xced/89223f37e2764c283767
    void *UIKit = dlopen([[[NSBundle bundleForClass:[UIApplication class]] executablePath] fileSystemRepresentation], RTLD_LAZY);
    UIAnimationDragCoefficient = dlsym(UIKit, "UIAnimationDragCoefficient");
}
#endif

- (instancetype)initWithCurve:(UIViewAnimationCurve)curve
                controlPoints:(NSArray<NSNumber *> *)controlPoints
                     duration:(NSTimeInterval)duration
        beginFromCurrentState:(BOOL)beginFromCurrentState
                    crossfade:(BOOL)crossfade
                    stiffness:(CGFloat)stiffness
                      damping:(CGFloat)damping
{
    self = [super init];

    if (self) {
        _curve = curve;
        _controlPoints = controlPoints;
        _duration = duration;
        _beginFromCurrentState = beginFromCurrentState;
        _crossfade = crossfade;
        _wasCancelled = NO;
        _stiffness = stiffness;
        _damping = damping;
        _pendingLayerAnimations = [NSMutableArray array];
        _runningLayerAnimations = [NSMutableArray array];
        _group = dispatch_group_create();
        _completions = [NSMutableArray array];
        _pendingAnimationsByLayer = [NSMutableDictionary dictionary];
        _disableRemoveOnComplete = NO;
    }

    return self;
}

- (void)_populateCAAnimation:(CAAnimation *)animation
{
    animation.removedOnCompletion = YES;
    animation.fillMode = kCAFillModeForwards;

    CASpringAnimation *springAnimation = ObjectAs(animation, CASpringAnimation);
    if (springAnimation) {
        springAnimation.stiffness = _stiffness;
        springAnimation.damping = _damping;
        springAnimation.duration = springAnimation.settlingDuration;
    } else {
#if TARGET_IPHONE_SIMULATOR
        animation.duration = _duration * MAX(1.0, UIAnimationDragCoefficient());
#else
        animation.duration = _duration;
#endif
        if (_controlPoints.count == 4) {
            animation.timingFunction = [CAMediaTimingFunction functionWithControlPoints:_controlPoints[0].floatValue :_controlPoints[1].floatValue :_controlPoints[2].floatValue :_controlPoints[3].floatValue];
        } else {
            switch (_curve) {
                case UIViewAnimationCurveLinear:
                    animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
                    break;
                case UIViewAnimationCurveEaseIn:
                    animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
                    break;
                case UIViewAnimationCurveEaseOut:
                    animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
                    break;
                case UIViewAnimationCurveEaseInOut:
                    animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
                    break;
            }
        }
    }
}

- (void)addTransitionOnLayer:(CALayer *)layer
{
    CAAnimation *existingAnimation = [layer animationForKey:@"transition"];

    if (existingAnimation && existingAnimation.delegate == nil) {
        // We will have an animation without a delegate if we just added
        // the transition without starting it yet. This means the animation
        // was already added from this animator.
        return;
    }

    CATransition *transition = [CATransition new];
    transition.type = kCATransitionFade;
    [self _populateCAAnimation:transition];

    [self _appendLayerAnimation:[[SCValdiLayerAnimation alloc] initWithLayer:layer animation:transition key:@"transition" disableRemoveOnComplete:_disableRemoveOnComplete]];
}

- (BOOL)_isSpringAnimation
{
    return _duration == 0;
}

- (void)_setValue:(id)value forKeyPath:(NSString *)keyPath inLayer:(CALayer *)layer
{
    if ([keyPath isEqualToString:@"bounds"]) {
        // Set the bounds on the backing UIView as well as to trigger
        // a layout invalidation
        UIView *view = ObjectAs(layer.delegate, UIView);
        if (view) {
            CGRect updatedBounds = [ObjectAs(value, NSValue) CGRectValue];

            if ([view requiresLayoutWhenAnimatingBounds] && !_crossfade) {
                dispatch_block_t animations = ^{
                    view.bounds = updatedBounds;
                    [view layoutIfNeeded];

                    // We only care about the animations for the children.
                    [view.layer removeAnimationForKey:@"bounds.origin"];
                    [view.layer removeAnimationForKey:@"bounds.size"];
                };
                
                id<UITimingCurveProvider> timingParameters;
                if ([self _isSpringAnimation]) {
                    timingParameters = [[UISpringTimingParameters alloc] initWithMass:1.0
                                                                            stiffness:_stiffness
                                                                              damping:_damping
                                                                      initialVelocity:CGVectorMake(0, 0)];
                } else {
                    if (_controlPoints.count == 4) {
                        CGPoint controlPoint1 = CGPointMake(_controlPoints[0].doubleValue, _controlPoints[1].doubleValue);
                        CGPoint controlPoint2 = CGPointMake(_controlPoints[2].doubleValue, _controlPoints[3].doubleValue);
                        timingParameters = [[UICubicTimingParameters alloc] initWithControlPoint1:controlPoint1
                                                                                    controlPoint2:controlPoint2];
                    } else {
                        timingParameters = [[UICubicTimingParameters alloc] initWithAnimationCurve:_curve];
                    }
                }
                
                // UIKit ignores the passed duration if timingParameters are UISpringTimingParameters
                UIViewPropertyAnimator *animator = [[UIViewPropertyAnimator alloc] initWithDuration:_duration
                                                                                   timingParameters:timingParameters];
                [animator addAnimations:animations];
                [animator startAnimation];
            } else {
                view.bounds = updatedBounds;
            }
        }
    }
    [layer setValue:value forKeyPath:keyPath];
}

- (CAAnimation *)_removeConflictingAnimationOnLayer:(CALayer *)layer forKeyPath:(NSString *)keyPath
{
    NSMutableArray<SCValdiLayerAnimation *> *pendingAnimations = [self _pendingAnimationsForLayer:layer];
    for (SCValdiLayerAnimation *layerAnimation in pendingAnimations) {
        if ([layerAnimation.key isEqualToString:keyPath]) {
            CAAnimation *animation = layerAnimation.animation;

            [pendingAnimations removeObject:layerAnimation];
            [_pendingLayerAnimations removeObject:layerAnimation];

            return animation;
        }
    }

    return nil;
}

- (void)addAnimationOnLayer:(CALayer *)layer forKeyPath:(NSString *)keyPath value:(id)value
{
    if (_crossfade) {
        [self addTransitionOnLayer:layer];
    } else {
        CAAnimation *removedAnimation = [self _removeConflictingAnimationOnLayer:layer forKeyPath:keyPath];

        id fromValue;
        if (!_beginFromCurrentState || ![layer animationForKey:keyPath]) {
            if ([removedAnimation isKindOfClass:[CABasicAnimation class]]) {
                // If we removed a conflicting animation, we take the fromValue
                // from the removed animation directly, as the fromValue of the layer
                // will reflect the end state of the removed animation.
                fromValue = [(CABasicAnimation *)removedAnimation fromValue];
            } else {
                fromValue = [layer valueForKeyPath:keyPath];
            }
        } else {
            fromValue = [layer.presentationLayer valueForKeyPath:keyPath];
        }
        
        CABasicAnimation *animation;
        if (_duration) {
            animation = [CABasicAnimation animationWithKeyPath:keyPath];
        } else {
            animation = [CASpringAnimation animationWithKeyPath:keyPath];
        }
        animation.fromValue = fromValue;
        animation.toValue = value;

        [self _populateCAAnimation:animation];

        [self _appendLayerAnimation:[[SCValdiLayerAnimation alloc] initWithLayer:layer animation:animation key:keyPath disableRemoveOnComplete:_disableRemoveOnComplete]];
    }

    [self _setValue:value forKeyPath:keyPath inLayer:layer];
}

- (void)addCompletion:(dispatch_block_t)completion
{
    [_completions addObject:completion];
}

- (NSMutableArray<SCValdiLayerAnimation *> *)_pendingAnimationsForLayer:(CALayer *)layer
{
    NSValue *key = [NSValue valueWithNonretainedObject:layer];
    return _pendingAnimationsByLayer[key];
}

- (void)_appendLayerAnimation:(SCValdiLayerAnimation *)layerAnimation
{
    [_pendingLayerAnimations addObject:layerAnimation];
    NSValue *key = [NSValue valueWithNonretainedObject:layerAnimation.layer];
    NSMutableArray<SCValdiLayerAnimation *> *animationsInLayer = _pendingAnimationsByLayer[key];
    if (!animationsInLayer) {
        animationsInLayer = [NSMutableArray array];
        _pendingAnimationsByLayer[key] = animationsInLayer;
    }

    [animationsInLayer addObject:layerAnimation];
}

static BOOL SCValdiLayerHasParentInHashTable(CALayer *layer, NSHashTable *table)
{
    CALayer *parent = layer.superlayer;
    if (!parent) {
        return NO;
    }

    if ([table containsObject:parent]) {
        return YES;
    }

    return SCValdiLayerHasParentInHashTable(parent, table);
}

- (void)_removeAnimationsFromChildrenIfNeeded
{
    NSHashTable *table = [[NSHashTable alloc] initWithOptions:NSPointerFunctionsStrongMemory capacity:_pendingLayerAnimations.count];

    // Index all our layers
    for (SCValdiLayerAnimation *animation in _pendingLayerAnimations) {
        [table addObject:animation.layer];
    }

    NSMutableArray<SCValdiLayerAnimation *> *updatedPendingLayerAnimations = [NSMutableArray array];

    for (SCValdiLayerAnimation *animation in _pendingLayerAnimations) {
        if (!SCValdiLayerHasParentInHashTable(animation.layer, table)) {
            [updatedPendingLayerAnimations addObject:animation];
        }
     }

    _pendingLayerAnimations = updatedPendingLayerAnimations;
}

- (void)flushAnimations:(NSObject *)completion
{
    if (_crossfade) {
        // CoreAnimation is able to transition with the children of a layer as well.
        // We thus need to remove the transitions that were added to children if their parent
        // already had a transition added.
        [self _removeAnimationsFromChildrenIfNeeded];
    }

    for (SCValdiLayerAnimation *pendingAnimation in _pendingLayerAnimations) {
        dispatch_group_enter(_group);
        [pendingAnimation performAnimationWithCompletion:^(BOOL finished) {
            // strong ref needed since no one retains this object
            dispatch_group_leave(self->_group);
        }];
        [_runningLayerAnimations addObject:pendingAnimation];
    }
    [_pendingLayerAnimations removeAllObjects];
    
    __weak SCValdiAnimator *weakSelf = self;
    if ([completion conformsToProtocol:@protocol(SCValdiFunction)]) {
        [self addCompletion:^{
            SCValdiMarshallerScoped(marshaller, {
                SCValdiMarshallerPushBool(marshaller, [weakSelf wasCancelled]);
                [((id<SCValdiFunction>)completion) performWithMarshaller:marshaller];
            })
        }];
    }

    NSArray *completions = _completions.copy;
    dispatch_group_notify(_group, dispatch_get_main_queue(), ^{
        for (dispatch_block_t completion in completions) {
            completion();
        }
        [weakSelf _removeAllRunningAnimations];
    });
}

- (void)_removeCompletedAnimation:(SCValdiLayerAnimation *)animation {
    [_runningLayerAnimations removeObject:animation];
}

- (void)_removeAllRunningAnimations {
    [_runningLayerAnimations removeAllObjects];
}

- (void)cancel
{
    // We're already cancelling, move along.
    if (_wasCancelled) {
        return;
    }
    _wasCancelled = YES;
    for (SCValdiLayerAnimation *animation in _runningLayerAnimations) {
        [animation cancelAnimationIfRunning];
    }
    [_runningLayerAnimations removeAllObjects];
}

-(void)setDisableRemoveOnComplete:(BOOL)disableRemoveOnComplete
{
    _disableRemoveOnComplete = disableRemoveOnComplete;
}


@end
