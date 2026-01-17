#import "valdi_core/SCValdiModuleFactoryRegistry.h"
#import <SCCHelloWorldTypes/SCCHelloWorldTypes.h>
#import <Foundation/Foundation.h>

@interface SCMyNativeModule: NSObject<SCCHelloWorldNativeModuleModule>

@end

@implementation SCMyNativeModule

- (NSString *)APP_NAME
{
    return @"Valdi iOS";
}

- (void)setAPP_NAME:(NSString *)appName
{

}

@end

@interface SCMyNativeModuleFactory : SCCHelloWorldNativeModuleModuleFactory

@end

@implementation SCMyNativeModuleFactory

VALDI_REGISTER_MODULE()

- (id<SCCHelloWorldNativeModuleModule>)onLoadModule
{
    return [SCMyNativeModule new];
}

@end