//
//  JavaScriptModuleFactory.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 12/13/22.
//

#pragma once

#include "valdi/runtime/JavaScript/Modules/JavaScriptModuleFactory.hpp"
#include "valdi_core/ModuleFactory.hpp"

namespace Valdi {

/**
 * Implementation of JavaScriptModuleFactory that bridges from an existing snap::valdi_core::ModuleFactory instance.
 */
class JavaScriptModuleFactoryBridge : public JavaScriptModuleFactory {
public:
    explicit JavaScriptModuleFactoryBridge(const Shared<snap::valdi_core::ModuleFactory>& moduleFactory);
    ~JavaScriptModuleFactoryBridge() override;

    StringBox getModulePath() const final;

    JSValueRef loadModule(IJavaScriptContext& jsContext,
                          const ReferenceInfoBuilder& referenceInfoBuilder,
                          JSExceptionTracker& exceptionTracker) override;

private:
    Shared<snap::valdi_core::ModuleFactory> _moduleFactory;
};

} // namespace Valdi
