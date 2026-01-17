#import "SCValdiActivityIndicatorView.h"

#import <valdi/ios/Categories/UIView+Valdi.h>

@implementation SCValdiActivityIndicatorView

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self _startAnimatingIfNeeded];
    }
    return self;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    [self _startAnimatingIfNeeded];
}

- (BOOL)willEnqueueIntoValdiPool
{
    return self.class == [SCValdiActivityIndicatorView class];
}

#pragma mark - Internal methods

// Defer the animation start until the view has a frame, so the loading indicator view's ease-in animation can be seen
- (void)_startAnimatingIfNeeded
{
    if (!self.isAnimating && !CGRectEqualToRect(self.bounds, CGRectZero)) {
        [self startAnimating];
    }
}

- (BOOL)_setColor:(UIColor *)color animator:(id<SCValdiAnimatorProtocol> )animator
{
    BOOL wasAnimating = self.isAnimating;
    SCValdiAnimatorTransitionWrap(animator, self, {
        self.color = color;
        if (wasAnimating) {
            [self startAnimating];
        }
    });
    return YES;
}

#pragma mark - Static methods

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAttribute:@"color"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(SCValdiActivityIndicatorView *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view _setColor:attributeValue animator:animator];
        }
        resetBlock:^(SCValdiActivityIndicatorView *view, id<SCValdiAnimatorProtocol> animator) {
            [view _setColor:[UIColor whiteColor] animator:animator];
        }];
}

@end
