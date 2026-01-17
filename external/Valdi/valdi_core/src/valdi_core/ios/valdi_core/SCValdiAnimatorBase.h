#import <UIKit/UIKit.h>

@protocol SCValdiAnimatorProtocol <NSObject>

@property (readonly, nonatomic) BOOL crossfade;

/**
 Add a CABasicAnimation to the layer at the given keyPath with the given toValue
 */
- (void)addAnimationOnLayer:(nonnull CALayer*)layer forKeyPath:(nonnull NSString*)keyPath value:(nullable id)value;

/**
 Add a UIKit's transition to the given view, applying the changes in the block
 */
- (void)addTransitionOnLayer:(nonnull CALayer*)layer;

- (void)addCompletion:(nonnull dispatch_block_t)completion;

- (nonnull instancetype)initWithCurve:(UIViewAnimationCurve)curve
                        controlPoints:(nullable NSArray<NSNumber*>*)controlPoints
                             duration:(NSTimeInterval)duration
                beginFromCurrentState:(BOOL)beginFromCurrentState
                            crossfade:(BOOL)crossfade
                            stiffness:(CGFloat)stiffness
                              damping:(CGFloat)damping;

@end

// DEPRECATED, USE -[SCValdiAnimator addTransitionOnLayer:]
#define SCValdiAnimatorTransitionWrap(animator, view, transitionBlock)                                                 \
    transitionBlock [animator addTransitionOnLayer:view.layer]
