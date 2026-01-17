//
//  JavaScriptTypes.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/11/19.
//

#include "valdi/runtime/JavaScript/JavaScriptTypes.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptTaskScheduler.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"

namespace Valdi {

void releaseRef(IJavaScriptContext& context, const JSValue& value) {
    context.releaseValue(value);
}

void retainRef(IJavaScriptContext& context, const JSValue& value) {
    context.retainValue(value);
}

void releaseRef(IJavaScriptContext& context, const JSPropertyName& propertyName) {
    context.releasePropertyName(propertyName);
}

void retainRef(IJavaScriptContext& context, const JSPropertyName& propertyName) {
    context.retainPropertyName(propertyName);
}

const ReferenceInfo& JSFunction::getReferenceInfo() const {
    static ReferenceInfo kEmptyReferenceInfo;
    return kEmptyReferenceInfo;
}

JSExceptionTracker::JSExceptionTracker(IJavaScriptContext& jsContext) : _jsContext(jsContext) {}

JSExceptionTracker::~JSExceptionTracker() {
    assertEmpty();
}

JSValueRef JSExceptionTracker::getExceptionAndClear() {
    assertNotEmpty();

    auto error = std::move(_error);
    auto exception = std::move(_exception);
    setEmpty(true);

    if (error) {
        exception = convertValdiErrorToJSError(_jsContext, error.value(), *this);
        if (!*this) {
            clearError();
        }
    }

    return exception;
}

void JSExceptionTracker::onClearError() {
    setEmpty(true);
    _exception = JSValueRef();
    _error = std::nullopt;
}

void JSExceptionTracker::onSetError(Error&& error) {
    _exception = JSValueRef();
    _error = {std::move(error)};
}

Error JSExceptionTracker::onGetError() const {
    if (_error) {
        return _error.value();
    } else {
        return convertJSErrorToValdiError(_jsContext, _exception, nullptr);
    }
}

JavaScriptCircularRefChecker<JSValue>& JSExceptionTracker::getCircularRefChecker() {
    return _circularRefChecker;
}

void JSExceptionTracker::storeException(JSValueRef&& exception) {
    assertEmpty();
    _exception = std::move(exception);
    if (!_exception.isRetained()) {
        _exception = JSValueRef::makeRetained(_jsContext, _exception.get());
    }
    setEmpty(false);
}

Result<JSValueRef> JSExceptionTracker::toRetainedResult(JSValueRef value) {
    if (!*this) {
        return extractError();
    }
    return _jsContext.ensureRetainedValue(std::move(value));
}

void JSExceptionTracker::storeException(const JSValue& exception) {
    storeException(JSValueRef::makeRetained(_jsContext, exception));
}

const Ref<JSStackTraceProvider>& JSExceptionTracker::getStackTraceProvider() const {
    return _stackTraceProvider;
}

void JSExceptionTracker::setStackTraceProvider(Ref<JSStackTraceProvider>&& stackTraceProvider) {
    _stackTraceProvider = std::move(stackTraceProvider);
}

} // namespace Valdi
