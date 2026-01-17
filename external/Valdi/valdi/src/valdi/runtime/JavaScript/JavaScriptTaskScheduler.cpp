//
//  JavaScriptTaskScheduler.cpp
//  valdi
//
//  Created by Simon Corsin on 5/12/21.
//

#include "valdi/runtime/JavaScript/JavaScriptTaskScheduler.hpp"

namespace Valdi {

JavaScriptEntryParameters::JavaScriptEntryParameters(IJavaScriptContext& jsContext,
                                                     JSExceptionTracker& exceptionTracker,
                                                     const Ref<Context>& valdiContext)
    : jsContext(jsContext), exceptionTracker(exceptionTracker), valdiContext(valdiContext) {
    jsContext.willEnterVM();
}

JavaScriptEntryParameters::~JavaScriptEntryParameters() {
    jsContext.willExitVM(exceptionTracker);
}

} // namespace Valdi
