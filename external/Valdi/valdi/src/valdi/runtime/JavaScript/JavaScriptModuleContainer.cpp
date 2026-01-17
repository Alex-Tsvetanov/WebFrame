//
//  JavaScriptModuleContainer.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 8/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/runtime/JavaScript/JavaScriptModuleContainer.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"
#include "valdi/runtime/JavaScript/JavaScriptValueMarshaller.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

JavaScriptModuleContainer::JavaScriptModuleContainer(Ref<Bundle> bundle,
                                                     IJavaScriptContext& context,
                                                     const JSValue& value,
                                                     JSExceptionTracker& exceptionTracker)
    : JSValueRefHolder(
          context, value, ReferenceInfoBuilder().withObject(bundle->getName()).build(), exceptionTracker, false),
      _bundle(std::move(bundle)) {}

JavaScriptModuleContainer::~JavaScriptModuleContainer() = default;

const Ref<Bundle>& JavaScriptModuleContainer::getBundle() const {
    return _bundle;
}

int32_t JavaScriptModuleContainer::pushToMarshaller(IJavaScriptContext& jsContext, Marshaller& marshaller) {
    JSExceptionTracker exceptionTracker(jsContext);
    auto jsValue = getJsValue(jsContext, exceptionTracker);
    if (!exceptionTracker) {
        marshaller.getExceptionTracker().onError(exceptionTracker.extractError());
        return 0;
    }

    Value convertedValue;
    auto* valueMarshaller = jsContext.getValueMarshaller();
    if (valueMarshaller != nullptr) {
        convertedValue = valueMarshaller->marshall(JSValueRef::makeUnretained(jsContext, jsValue),
                                                   marshaller.getCurrentSchema(),
                                                   ReferenceInfoBuilder(getReferenceInfo()),
                                                   exceptionTracker);
    } else {
        convertedValue = jsValueToValue(jsContext, jsValue, ReferenceInfoBuilder(getReferenceInfo()), exceptionTracker);
    }

    if (!exceptionTracker) {
        marshaller.getExceptionTracker().onError(exceptionTracker.extractError());
        return 0;
    }

    return marshaller.push(std::move(convertedValue));
}

static void callUnloadCallback(const Value& observer) {
    auto* func = observer.getFunction();
    if (func != nullptr) {
        (*func)();
    }
}

void JavaScriptModuleContainer::addUnloadObserver(const Valdi::Value& observer) {
    if (_isUnloaded) {
        callUnloadCallback(observer);
    } else {
        _unloadObservers.emplace_back(observer);
    }
}

void JavaScriptModuleContainer::didUnload() {
    _isUnloaded = true;
    auto callbacks = _unloadObservers;
    _unloadObservers.clear();
    for (const auto& callback : callbacks) {
        callUnloadCallback(callback);
    }

    auto lock = getContext()->lock();
    dispose(lock);
}

} // namespace Valdi
