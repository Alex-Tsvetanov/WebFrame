//
//  JSCoreCustomClasses.hpp
//  ValdiIOS
//
//  Created by Simon Corsin on 5/31/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/JavaScript/JavaScriptTypes.hpp"
#include <JavaScriptCore/JavaScriptCore.h>

namespace ValdiJSCore {

JSClassRef getNativeFunctionClassRef();
JSClassRef getWrappedObjectClassRef();

class JavaScriptCoreContext;

struct JSFunctionData : public Valdi::SimpleRefCountable {
public:
    JavaScriptCoreContext& jsContext;
    Valdi::Ref<Valdi::JSFunction> function;

    JSFunctionData(JavaScriptCoreContext& jsContext, const Valdi::Ref<Valdi::JSFunction>& function);
    ~JSFunctionData() override;
};

JSFunctionData* getAttachedJsFunctionData(JSObjectRef objectRef);
Valdi::RefCountable* getAttachedWrappedObject(JSObjectRef objectRef);

} // namespace ValdiJSCore
