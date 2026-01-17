#import "valdi_core/SCValdiGeneratedModuleFactory.h"
#import "valdi_core/SCValdiMarshallableObjectRegistry.h"

@implementation SCValdiGeneratedModuleFactory

- (Protocol *)moduleProtocol
{
    return nil;
}

- (id)onLoadModule
{
    [NSException raise:NSInternalInconsistencyException format:@"onLoadModule must be overriden by subclasses"];
    return nil;
}

- (nonnull NSString *)getModulePath
{
    [NSException raise:NSInternalInconsistencyException format:@"getModulePath must be overriden by subclasses"];
    return nil;
}

- (nonnull NSObject *)loadModule
{
    id module = [self onLoadModule];
    Protocol *protocol = [self moduleProtocol];
    if (![module conformsToProtocol:protocol]) {
        [NSException raise:NSInternalInconsistencyException format:@"onLoadModule must return an instance of protocol %@ (received: %@)", protocol, [module class]];
        return nil;
    }

    SCValdiMarshallerObjectRegistryRegisterProxyIfNeeded(module, protocol);

    return module;
}

@end
