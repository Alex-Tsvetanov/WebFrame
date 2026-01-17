//
//  QuickJSJavaScriptContextFactory.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/19/19.
//

#include "valdi/quickjs/QuickJSJavaScriptContextFactory.hpp"
#include "valdi/quickjs/QuickJSJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptHeapDumpBuilder.hpp"

namespace ValdiQuickJS {

QuickJSJavaScriptContextFactory::QuickJSJavaScriptContextFactory() = default;

const char* QuickJSJavaScriptContextFactory::getName() {
    return "QuickJS";
}

bool QuickJSJavaScriptContextFactory::requiresDedicatedThread() {
    return true;
}

Valdi::Ref<Valdi::IJavaScriptContext> QuickJSJavaScriptContextFactory::createJsContext(
    Valdi::JavaScriptTaskScheduler* taskScheduler, Valdi::ILogger& /*logger*/) {
    return Valdi::makeShared<QuickJSJavaScriptContext>(taskScheduler);
}

Valdi::BytesView QuickJSJavaScriptContextFactory::dumpHeap(std::span<Valdi::IJavaScriptContext*> jsContexts,
                                                           Valdi::JSExceptionTracker& exceptionTracker) {
    if constexpr (Valdi::shouldEnableJsHeapDump()) {
        Valdi::JavaScriptHeapDumpBuilder heapDumpBuilder;

        for (auto* jsContext : jsContexts) {
            dynamic_cast<QuickJSJavaScriptContext*>(jsContext)->dumpHeap(heapDumpBuilder);
        }

        return heapDumpBuilder.build();
    } else {
        exceptionTracker.onError("Heap dump support not enabled");
        return Valdi::BytesView();
    }
}

} // namespace ValdiQuickJS
