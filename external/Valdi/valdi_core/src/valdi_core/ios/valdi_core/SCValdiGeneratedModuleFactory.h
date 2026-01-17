#import "valdi_core/SCValdiModuleFactoryRegistry.h"
#import <Foundation/Foundation.h>

@interface SCValdiGeneratedModuleFactory : NSObject <SCNValdiCoreModuleFactory>

- (id)onLoadModule;

- (Protocol*)moduleProtocol;

@end
