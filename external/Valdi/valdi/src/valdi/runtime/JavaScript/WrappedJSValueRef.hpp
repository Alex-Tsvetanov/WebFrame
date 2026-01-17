//
//  WrappedJSValueRef.hpp
//  valdi
//
//  Created by Simon Corsin on 5/11/21.
//

#pragma once

#include "valdi/runtime/JavaScript/JSValueRefHolder.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

class WrappedJSValueRef : public ValdiObject, public JSValueRefHolder {
public:
    WrappedJSValueRef(IJavaScriptContext& context,
                      const JSValue& value,
                      const ReferenceInfo& referenceInfo,
                      JSExceptionTracker& exceptionTracker);
    ~WrappedJSValueRef() override;

    VALDI_CLASS_HEADER(WrappedJSValueRef)
};

} // namespace Valdi
