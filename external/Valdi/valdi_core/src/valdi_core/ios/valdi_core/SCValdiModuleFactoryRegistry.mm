#import "valdi_core/SCValdiModuleFactoryRegistry.h"
#import "valdi_core/cpp/JavaScript/ModuleFactoryRegistry.hpp"
#import "valdi_core/SCNValdiCoreModuleFactory+Private.h"

FOUNDATION_EXPORT void SCValdiModuleFactoryRegisterWithBlock(SCValdiModuleFactoryBlock block)
{
    Valdi::ModuleFactoryRegistry::sharedInstance()->registerModuleFactoryFn([block]() {
        id<SCNValdiCoreModuleFactory> moduleFactory = block();
        return djinni_generated_client::valdi_core::ModuleFactory::toCpp(moduleFactory);
    });
}

FOUNDATION_EXPORT void SCValdiModuleFactoryRegisterWithClass(Class cls)
{
    SCValdiModuleFactoryRegisterWithBlock(^{
        return [cls new];
    });
}