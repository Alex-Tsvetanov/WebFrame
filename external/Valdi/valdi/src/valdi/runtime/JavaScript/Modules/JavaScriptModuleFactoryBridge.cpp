//
//  JavaScriptModuleFactory.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 12/13/22.
//

#include "valdi/runtime/JavaScript/Modules/JavaScriptModuleFactoryBridge.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"

namespace Valdi {

JavaScriptModuleFactoryBridge::JavaScriptModuleFactoryBridge(
    const Shared<snap::valdi_core::ModuleFactory>& moduleFactory)
    : _moduleFactory(moduleFactory) {}
JavaScriptModuleFactoryBridge::~JavaScriptModuleFactoryBridge() = default;

StringBox JavaScriptModuleFactoryBridge::getModulePath() const {
    return _moduleFactory->getModulePath();
}

JSValueRef JavaScriptModuleFactoryBridge::loadModule(IJavaScriptContext& jsContext,
                                                     const ReferenceInfoBuilder& referenceInfoBuilder,
                                                     JSExceptionTracker& exceptionTracker) {
    auto value = _moduleFactory->loadModule();
    return valueToJSValue(jsContext, value, referenceInfoBuilder.withReturnValue(), exceptionTracker);
}

} // namespace Valdi
