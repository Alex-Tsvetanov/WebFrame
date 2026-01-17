#import "SwiftValdiRuntimeManager.h"
#import "valdi/ios/SCValdiRuntimeManager.h"
#import "valdi_core/SCValdiRuntimeManagerProtocol.h"

@interface SwiftValdiRuntimeManager ()
@property (nonatomic, strong) SCValdiRuntimeManager* runtimeManager;
@end

@implementation SwiftValdiRuntimeManager

- (id<SCValdiRuntimeManagerProtocol>)createRuntimeManager {
    if (!self.runtimeManager) {
        self.runtimeManager = [[SCValdiRuntimeManager alloc] init];
    }
    return self.runtimeManager;
}

- (id<SCValdiRuntimeProtocol>)createRuntime {
    return self.runtimeManager.mainRuntime;
}

@end
