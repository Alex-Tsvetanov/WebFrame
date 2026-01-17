//
//  JSCoreCustomClasses.cpp
//  ValdiIOS
//
//  Created by Simon Corsin on 5/31/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/jscore/JSCoreCustomClasses.hpp"
#include "valdi/jscore/JSCoreUtils.hpp"
#include "valdi/jscore/JavaScriptCoreContext.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"
#include "valdi/runtime/Utils/RefCountableAutoreleasePool.hpp"
#include "valdi_core/cpp/Constants.hpp"

namespace ValdiJSCore {

static JSValueRef onJsCallError(JSContextRef ctx,
                                Valdi::JSExceptionTracker& exceptionTracker,
                                JSValueRef* exceptionPtr) {
    auto exception = exceptionTracker.getExceptionAndClear();
    *exceptionPtr = fromValdiJSValue(exception.get()).valueRef;
    return JSValueMakeUndefined(ctx);
}

JSValueRef callAsFunction(JSContextRef ctx,
                          JSObjectRef function,
                          JSObjectRef thisObject,
                          size_t argumentCount,
                          const JSValueRef arguments[],
                          JSValueRef* exception) {
    auto* attachedJsFunctionData = getAttachedJsFunctionData(function);
    auto& jsContext = attachedJsFunctionData->jsContext;

    Valdi::JSValueRef outArguments[argumentCount];
    for (size_t i = 0; i < argumentCount; i++) {
        outArguments[i] = Valdi::JSValueRef::makeUnretained(
            jsContext, toValdiJSValue(JSCoreRef(arguments[i], JSValueGetType(ctx, arguments[i]))));
    }

    Valdi::JSExceptionTracker exceptionTracker(attachedJsFunctionData->jsContext);
    Valdi::JSFunctionNativeCallContext callContext(attachedJsFunctionData->jsContext,
                                                   outArguments,
                                                   static_cast<size_t>(argumentCount),
                                                   exceptionTracker,
                                                   attachedJsFunctionData->function->getReferenceInfo());
    callContext.setThisValue(toValdiJSValue(JSCoreRef(thisObject, kJSTypeObject)));

    if (attachedJsFunctionData->jsContext.interruptRequested()) {
        attachedJsFunctionData->jsContext.onInterrupt();
    }

    auto result = (*attachedJsFunctionData->function)(callContext);

    if (VALDI_LIKELY(exceptionTracker)) {
        return fromValdiJSValue(result.get()).valueRef;
    } else {
        return onJsCallError(ctx, exceptionTracker, exception);
    }
}

void finalize(JSObjectRef object) {
    Valdi::RefCountableAutoreleasePool::release(JSObjectGetPrivate(object));
}

JSClassRef getNativeFunctionClassRef() {
    static JSClassRef _nativeFunctionClassRef = nullptr;
    if (_nativeFunctionClassRef == nullptr) {
        auto classDefinition = kJSClassDefinitionEmpty;
        classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
        classDefinition.callAsFunction = &callAsFunction;
        classDefinition.finalize = &finalize;

        _nativeFunctionClassRef = JSClassCreate(&classDefinition);
    }

    return _nativeFunctionClassRef;
}

JSClassRef getWrappedObjectClassRef() {
    static JSClassRef _wrappedObjectClassRef = nullptr;
    if (_wrappedObjectClassRef == nullptr) {
        auto wrappedObjectClsDefinition = kJSClassDefinitionEmpty;
        wrappedObjectClsDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
        wrappedObjectClsDefinition.finalize = &finalize;

        _wrappedObjectClassRef = JSClassCreate(&wrappedObjectClsDefinition);
    }

    return _wrappedObjectClassRef;
}

JSFunctionData::JSFunctionData(JavaScriptCoreContext& jsContext, const Valdi::Ref<Valdi::JSFunction>& function)
    : jsContext(jsContext), function(function) {}
JSFunctionData::~JSFunctionData() = default;

inline Valdi::RefCountable* getAttachedRefCountable(JSObjectRef objectRef) {
    return reinterpret_cast<Valdi::RefCountable*>(JSObjectGetPrivate(objectRef));
}

JSFunctionData* getAttachedJsFunctionData(JSObjectRef objectRef) {
    return dynamic_cast<JSFunctionData*>(getAttachedRefCountable(objectRef));
}

Valdi::RefCountable* getAttachedWrappedObject(JSObjectRef objectRef) {
    auto* refCountable = getAttachedRefCountable(objectRef);
    if (dynamic_cast<JSFunctionData*>(refCountable) != nullptr) {
        // If our private data is a JSFunctionData, we don't consider it as a wrapped object
        return nullptr;
    }
    return refCountable;
}

} // namespace ValdiJSCore
