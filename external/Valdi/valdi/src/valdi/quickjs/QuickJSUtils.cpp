//
//  QuickJSUtils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/19/19.
//

#include "valdi/quickjs/QuickJSUtils.hpp"

#include "valdi/runtime/JavaScript/JSValueRefHolder.hpp"
#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"
#include "valdi_core/cpp/Constants.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include "valdi/quickjs/QuickJSJavaScriptContext.hpp"

#include <mutex>

namespace ValdiQuickJS {

static JSValue onJsCallError(JSContext* context, Valdi::JSExceptionTracker& exceptionTracker) {
    auto exception = exceptionTracker.getExceptionAndClear();
    return JS_Throw(context, JS_DupValue(context, fromValdiJSValue(exception.get())));
}

JSValue jsCall(
    JSContext* context, JSValueConst funcObject, JSValueConst thisValue, int argc, JSValueConst* argv, int /*flags*/) {
    auto& valdiJsContext = *getValdiJSContext(context);
    auto* function = getObjectCallable(funcObject);

    // Note: for even faster call performance, we could have Valdi::JSValue being the exact
    // same size as QuickJS's JSValue, and just pass the argv pointer directly.
    Valdi::JSValueRef arguments[argc];
    for (int i = 0; i < argc; i++) {
        arguments[i] = Valdi::JSValueRef::makeUnretained(valdiJsContext, toValdiJSValue(argv[i]));
    }

    Valdi::JSExceptionTracker exceptionTracker(valdiJsContext);
    Valdi::JSFunctionNativeCallContext callContext(
        valdiJsContext, arguments, static_cast<size_t>(argc), exceptionTracker, function->getReferenceInfo());
    callContext.setThisValue(toValdiJSValue(thisValue));

    if (valdiJsContext.interruptRequested()) {
        valdiJsContext.onInterrupt();
    }

    auto result = (*function)(callContext);

    if (VALDI_LIKELY(exceptionTracker)) {
        return JS_DupValue(context, fromValdiJSValue(result.get()));
    } else {
        return onJsCallError(context, exceptionTracker);
    }
}

void jsWrappedObjectFinalize(JSRuntime* /*tr*/, JSValue value) {
    setObjectWrappedObject(value, nullptr);
}

void jsFunctionFinalize(JSRuntime* /*tr*/, JSValue value) {
    setObjectCallable(value, nullptr);
}

void jsWeakRefFinalizerFinalize(JSRuntime* tr, JSValue value) {
    auto* jsContext = getValdiJSContext(tr);
    if (jsContext != nullptr) {
        auto weakReferenceId = weakReferenceIdFromJSWeakReferenceFinalizer(value);
        jsContext->removeWeakReference(weakReferenceId);
    }
}

JSClassID newClassID() {
    static auto* kMutex = new std::mutex();

    auto lockGuard = std::lock_guard<std::mutex>(*kMutex);

    JSClassID classID = 0;
    return JS_NewClassID(&classID);
}

JSClassDefWithId* newClassDefWithId(const char* className) {
    auto* classDefWithId = new JSClassDefWithId();
    std::memset(classDefWithId, 0, sizeof(JSClassDefWithId));

    classDefWithId->classID = newClassID();
    classDefWithId->classDef.class_name = className;

    return classDefWithId;
}

JSClassDefWithId* makeBridgedFunctionClassDef() {
    auto* classDefWithId = newClassDefWithId("NativeBridgedFunction");

    classDefWithId->classDef.call = &jsCall;
    classDefWithId->classDef.finalizer = &jsFunctionFinalize;

    return classDefWithId;
}

const JSClassDefWithId* getBridgedFunctionClassDef() {
    static auto* kClassDef = makeBridgedFunctionClassDef();

    return kClassDef;
}

JSClassDefWithId* makeWrappedObjectClassDef() {
    auto* classDefWithId = newClassDefWithId("WrappedNativeObject");

    classDefWithId->classDef.finalizer = &jsWrappedObjectFinalize;

    return classDefWithId;
}

const JSClassDefWithId* getWrappedObjectClassDef() {
    static auto* kClassDef = makeWrappedObjectClassDef();

    return kClassDef;
}

JSClassDefWithId* makeWeakRefFinalizerClassDef() {
    auto* classDefWithId = newClassDefWithId("WeakRefFinalizer");

    classDefWithId->classDef.finalizer = &jsWeakRefFinalizerFinalize;

    return classDefWithId;
}

const JSClassDefWithId* getWeakRefFinalizerClassDef() {
    static auto* kClassDef = makeWeakRefFinalizerClassDef();

    return kClassDef;
}

void setObjectWrappedObject(const JSValue& value, const Valdi::Ref<Valdi::RefCountable>& wrappedObject) {
    auto* opaque = JS_GetOpaque(value, getWrappedObjectClassDef()->classID);
    if (opaque != nullptr) {
        Valdi::RefCountableAutoreleasePool::release(opaque);
    }

    JS_SetOpaque(value, Valdi::unsafeBridgeRetain(wrappedObject.get()));
}

Valdi::Ref<Valdi::RefCountable> getObjectWrappedObject(const JSValue& value) {
    auto* opaque = JS_GetOpaque(value, getWrappedObjectClassDef()->classID);

    return Valdi::unsafeBridge<Valdi::RefCountable>(opaque);
}

size_t weakReferenceIdFromJSWeakReferenceFinalizer(const JSValue& jsWeakReferenceFinalizer) {
    auto* opaque = JS_GetOpaque(jsWeakReferenceFinalizer, getWeakRefFinalizerClassDef()->classID);
    return reinterpret_cast<size_t>(opaque);
}

void setWeakReferenceIdToJSWeakReferenceFinalizer(const JSValue& jsWeakReferenceFinalizer, size_t weakReferenceId) {
    JS_SetOpaque(jsWeakReferenceFinalizer, reinterpret_cast<void*>(weakReferenceId));
}

} // namespace ValdiQuickJS
