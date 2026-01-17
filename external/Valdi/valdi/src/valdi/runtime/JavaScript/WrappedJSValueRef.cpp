//
//  WrappedJSValueRef.cpp
//  valdi
//
//  Created by Simon Corsin on 5/11/21.
//

#include "valdi/runtime/JavaScript/WrappedJSValueRef.hpp"

namespace Valdi {

WrappedJSValueRef::WrappedJSValueRef(IJavaScriptContext& context,
                                     const JSValue& value,
                                     const ReferenceInfo& referenceInfo,
                                     JSExceptionTracker& exceptionTracker)
    : JSValueRefHolder(context, value, referenceInfo, exceptionTracker, true) {}

// See explanation in JSValueRefHolder.cpp
__attribute__((no_sanitize("thread"))) WrappedJSValueRef::~WrappedJSValueRef() = default;

VALDI_CLASS_IMPL(WrappedJSValueRef)

} // namespace Valdi
