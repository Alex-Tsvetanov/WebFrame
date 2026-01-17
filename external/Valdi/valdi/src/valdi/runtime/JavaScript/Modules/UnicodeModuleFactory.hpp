//
//  UnicodeModuleFactory.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/9/24.
//

#pragma once

#include "valdi/runtime/JavaScript/Modules/JavaScriptModuleFactory.hpp"

namespace Valdi {

class IJavaScriptContext;

class UnicodeModuleFactory : public JavaScriptModuleFactory {
public:
    UnicodeModuleFactory();
    ~UnicodeModuleFactory() override;

    StringBox getModulePath() const final;
    JSValueRef loadModule(IJavaScriptContext& context,
                          const ReferenceInfoBuilder& referenceInfoBuilder,
                          JSExceptionTracker& exceptionTracker) override;

private:
    static JSValueRef strToCodepoints(JSFunctionNativeCallContext& callContext);
    static JSValueRef codepointsToStr(JSFunctionNativeCallContext& callContext);
    static JSValueRef encodeString(JSFunctionNativeCallContext& callContext);
    static JSValueRef decodeIntoString(JSFunctionNativeCallContext& callContext);
};

} // namespace Valdi
