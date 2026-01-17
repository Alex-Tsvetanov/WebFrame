#import "valdi_core/SCValdiNoAnimationDelegate.h"

@implementation SCValdiNoAnimationDelegate

- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return [NSNull null];
}

+ (instancetype)sharedInstance
{
    static dispatch_once_t onceToken;
    static SCValdiNoAnimationDelegate *delegate;
    dispatch_once(&onceToken, ^{
        delegate = [SCValdiNoAnimationDelegate new];
    });
    return delegate;
}

@end