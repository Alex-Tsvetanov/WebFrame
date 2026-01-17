//
//  JSCoreUtils.hpp
//  ValdiIOS
//
//  Created by Simon Corsin on 5/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/JavaScript/JavaScriptTypes.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include <JavaScriptCore/JavaScriptCore.h>
#include <memory>

namespace ValdiJSCore {

std::pair<const char*, size_t> jsStringToUTF8(JSStringRef stringRef);
Valdi::StringBox jsStringtoStdString(JSStringRef stringRef);

struct JSCoreRef {
    JSValueRef valueRef;
    JSType type;

    inline JSCoreRef() = default;
    inline JSCoreRef(JSValueRef valueRef, JSType type) : valueRef(valueRef), type(type) {}

    constexpr bool isObject() const {
        return type == kJSTypeObject;
    }

    inline JSObjectRef asObjectRef() const {
        return isObject() ? const_cast<JSObjectRef>(valueRef) : nullptr;
    }

    JSObjectRef asObjectRefOrThrow(Valdi::IJavaScriptContext& jsContext,
                                   Valdi::JSExceptionTracker& exceptionTracker) const;
};

inline Valdi::JSValue toValdiJSValue(const JSCoreRef& value) {
    return Valdi::JSValue(value);
}

inline JSCoreRef fromValdiJSValue(const Valdi::JSValue& jsValue) {
    return jsValue.bridge<JSCoreRef>();
}

inline Valdi::JSPropertyName toValdiJSPropertyName(const JSStringRef& value) {
    return Valdi::JSPropertyName(value);
}

inline JSStringRef fromValdiJSPropertyName(const Valdi::JSPropertyName& value) {
    return value.bridge<JSStringRef>();
}

} // namespace ValdiJSCore
