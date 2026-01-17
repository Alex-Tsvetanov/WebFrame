#import "valdi/ios/Bootstrap/SCValdiAppMain.h"
#import "valdi/ios/Bootstrap/SCValdiBootstrappingAppDelegate.h"

static NSString *kRootComponentPath = nil;
@interface SCValdiAppMainDelegate: SCValdiBootstrappingAppDelegate

@end

@implementation SCValdiAppMainDelegate

- (NSString *)rootValdiComponentPath
{
    return kRootComponentPath;
}

@end

int SCValdiAppMain(int argc, char *argv[], NSString *rootComponentPath)
{
    @autoreleasepool {
        kRootComponentPath = rootComponentPath;
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([SCValdiAppMainDelegate class]));
    }
}