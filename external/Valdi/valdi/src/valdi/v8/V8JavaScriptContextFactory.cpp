//
//  V8JavaScriptContextFactory.hpp
//  valdi-v8
//
//  Created by Ramzy Jaber on 6/13/23.
//

#include "valdi/v8/V8JavaScriptContextFactory.hpp"
#include "valdi/v8/V8JavaScriptContext.hpp"

namespace Valdi::V8 {

Ref<IJavaScriptContext> V8JavaScriptContextFactory::createJsContext(JavaScriptTaskScheduler* taskScheduler,
                                                                    ILogger& /*logger*/) {
    return makeShared<V8JavaScriptContext>(taskScheduler);
}

BytesView V8JavaScriptContextFactory::dumpHeap(std::span<IJavaScriptContext*> /*jsContexts*/,
                                               JSExceptionTracker& exceptionTracker) {
    exceptionTracker.onError("V8 does not yet support heap dumps");
    return BytesView();
}

} // namespace Valdi::V8
