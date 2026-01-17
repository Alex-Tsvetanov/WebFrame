//
//  SCValdiJSAction.m
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#import "valdi/ios/Action/SCValdiJSAction.h"

#import "valdi_core/SCValdiActionUtils.h"
#import "valdi_core/SCValdiLogger.h"

#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/cpp/Utils/Value.hpp"

@implementation SCValdiJSAction {
    ObjCppPtrWrapper<Valdi::JavaScriptRuntime> _jsRuntime;
    Valdi::StringBox _functionName;
    uint32_t _objectID;
}

- (instancetype)initWithJSRuntime:(ObjCppPtrWrapper<Valdi::JavaScriptRuntime>)jsRuntime
                     functionName:(const Valdi::StringBox &)functionName
                         objectID:(uint32_t)objectID
{
    self = [super init];

    if (self) {
        _jsRuntime = jsRuntime;
        _functionName = functionName;
        _objectID = objectID;
    }

    return self;
}

- (void)performWithSender:(id)sender
{
    [self performWithParameters:SCValdiActionParametersWithSender(sender)];
}

- (id)performWithParameters:(NSArray<id> *)parameters
{
    auto value = ValdiIOS::ValueFromNSObject(parameters);
    _jsRuntime.o->callComponentFunction(Valdi::ContextId(_objectID), _functionName, value.getArrayRef());
    return nil;
}

@end
