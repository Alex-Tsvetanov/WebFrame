
#import "valdi_core/SCValdiRuntimeManagerProtocol.h"

@interface SwiftValdiRuntimeManager : NSObject

- (id<SCValdiRuntimeManagerProtocol>)createRuntimeManager;

- (id<SCValdiRuntimeProtocol>)createRuntime;

@end
