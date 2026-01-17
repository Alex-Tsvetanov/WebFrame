#import "valdi_core/SCNValdiCoreModuleFactory.h"
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VALDI_REGISTER_MODULE()                                                                                        \
    +(void)load {                                                                                                      \
        SCValdiModuleFactoryRegisterWithClass(self);                                                                   \
    }

typedef id<SCNValdiCoreModuleFactory> (^SCValdiModuleFactoryBlock)();

void SCValdiModuleFactoryRegisterWithBlock(SCValdiModuleFactoryBlock block);
void SCValdiModuleFactoryRegisterWithClass(Class cls);

#ifdef __cplusplus
}
#endif