//
//  JavaScriptModuleFactory.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 12/13/22.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"

namespace Valdi {

/**
 * A JavaScriptModuleFactory is an interface representing an object
 * that can load a JavaScript module within a JavaScript Runtime Environment.
 * This is typically used to build modules in native.
 */
class JavaScriptModuleFactory : public SimpleRefCountable {
public:
    /**
     * Return the path of the module, which represents its path
     * when imported from JavaScript through a require() call.
     */
    virtual StringBox getModulePath() const = 0;

    /**
     * Load the module, using the given jsContext and exceptionTracker to populate objects and functions.
     * Returns the object which should be used as the "exports" for the module (e.g. return value of
     * require('ModulePath'))
     */
    virtual JSValueRef loadModule(IJavaScriptContext& jsContext,
                                  const ReferenceInfoBuilder& referenceInfoBuilder,
                                  JSExceptionTracker& exceptionTracker) = 0;
};

} // namespace Valdi
