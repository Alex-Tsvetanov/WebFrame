//
//  SCValdiRuntime+Private.h
//  Valdi
//
//  Created by Simon Corsin on 1/8/19.
//

#ifdef __cplusplus

#import "valdi/ios/SCValdiRuntime.h"
#import "valdi/runtime/Context/ViewManagerContext.hpp"
#import "valdi/runtime/Runtime.hpp"

@class SCValdiFontManager;

@interface SCValdiRuntime ()

@property (readonly, nonatomic) const Valdi::SharedRuntime& cppInstance;

- (instancetype)initWithCppInstance:(const Valdi::SharedRuntime&)cppInstance
                 viewManagerContext:(const Valdi::Ref<Valdi::ViewManagerContext>&)viewManagerContext
                     runtimeManager:(id<SCValdiRuntimeManagerProtocol>)runtimeManager
                        fontManager:(SCValdiFontManager*)fontManager;

- (void)setAllowDarkMode:(BOOL)allowDarkMode
    useScreenUserInterfaceStyleForDarkMode:(BOOL)useScreenUserInterfaceStyleForDarkMode;

@end

#endif
