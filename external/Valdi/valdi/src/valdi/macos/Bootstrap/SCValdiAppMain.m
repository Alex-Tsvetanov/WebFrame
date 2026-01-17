#import "valdi/macos/Bootstrap/SCValdiAppMain.h"
#import "valdi/macos/Bootstrap/SCValdiBootstrappingNSAppDelegate.h"

static NSString *kRootComponentPath = nil;
@interface SCValdiAppMainDelegate: SCValdiBootstrappingNSAppDelegate

@end

@implementation SCValdiAppMainDelegate

- (NSString *)rootValdiComponentPath
{
    return kRootComponentPath;
}

@end

#ifdef __cplusplus
extern "C" {
#endif

void SCValdiAppMain(int argc,
                    const char **argv,
                    NSString *componentPath,
                    NSString *title,
                    int windowWidth,
                    int windowHeight,
                    bool windowResizable
                   )
{
    @autoreleasepool {
        SCValdiBootstrappingNSAppDelegate *appDelegate =
            [[SCValdiBootstrappingNSAppDelegate alloc]
                 initWithRootValdiComponentPath:componentPath 
                 title:title
                 windowWidth: windowWidth
                 windowHeight: windowHeight
                 windowResizable: windowResizable
            ];
        NSApplication *application = [NSApplication sharedApplication];
        application.delegate = appDelegate;
        [application run];
    }
}

#ifdef __cplusplus
}
#endif
