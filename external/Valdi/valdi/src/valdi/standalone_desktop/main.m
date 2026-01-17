#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        AppDelegate *appDelegate = [AppDelegate new];
        NSApplication *application = [NSApplication sharedApplication];
        application.delegate = appDelegate;
        [application run];
    }
    return 0;
}
