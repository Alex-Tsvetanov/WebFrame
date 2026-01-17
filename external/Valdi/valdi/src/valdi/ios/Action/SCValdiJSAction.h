//
//  SCValdiJSAction.h
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#ifdef __cplusplus

#import "valdi_core/SCMacros.h"

#import "valdi/runtime/JavaScript/JavaScriptRuntime.hpp"
#import "valdi_core/SCValdiAction.h"
#import "valdi_core/SCValdiFunctionCompat.h"
#import <string>
#import <utils/ObjCppPtrWrapper.hpp>

#import <Foundation/Foundation.h>

@interface SCValdiJSAction : SCValdiFunctionCompat <SCValdiAction>

VALDI_NO_INIT

- (instancetype)initWithJSRuntime:(ObjCppPtrWrapper<Valdi::JavaScriptRuntime>)jsRuntime
                     functionName:(const Valdi::StringBox&)functionName
                         objectID:(uint32_t)objectID;

@end

#endif