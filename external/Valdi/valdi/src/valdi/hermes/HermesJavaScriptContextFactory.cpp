//
//  HermesJavaScriptContextFactory.cpp
//  valdi-hermes
//
//  Created by Simon Corsin on 9/27/23.
//

#include "valdi/hermes/HermesJavaScriptContextFactory.hpp"
#include "valdi/hermes/HermesJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptHeapDumpBuilder.hpp"

#ifdef HERMES_ENABLE_DEBUGGER
#include "valdi/hermes/HermesDebuggerServer.hpp"
#endif

namespace Valdi::Hermes {

HermesJavaScriptContextFactory::HermesJavaScriptContextFactory() = default;

const char* HermesJavaScriptContextFactory::getName() {
    return "Hermes";
}

bool HermesJavaScriptContextFactory::requiresDedicatedThread() {
    return true;
}

Ref<IJavaScriptContext> HermesJavaScriptContextFactory::createJsContext(JavaScriptTaskScheduler* taskScheduler,
                                                                        ILogger& logger) {
    return makeShared<HermesJavaScriptContext>(taskScheduler, logger);
}

void HermesJavaScriptContextFactory::startJsDebuggerServer([[maybe_unused]] ILogger& logger) {
#ifdef HERMES_ENABLE_DEBUGGER
    if (_debuggerServer == nullptr) {
        _debuggerServer = HermesDebuggerServer::getInstance(logger);
    }
    if (_debuggerServer != nullptr) {
        _debuggerServer->start();
    }
#else
    return;
#endif
}

void HermesJavaScriptContextFactory::stopJsDebuggerServer() {
#ifdef HERMES_ENABLE_DEBUGGER
    if (_debuggerServer != nullptr) {
        _debuggerServer->stop();
    }
#else
    return;
#endif
}

BytesView HermesJavaScriptContextFactory::dumpHeap(std::span<IJavaScriptContext*> jsContexts,
                                                   JSExceptionTracker& exceptionTracker) {
    if constexpr (shouldEnableJsHeapDump()) {
        Valdi::JavaScriptHeapDumpBuilder heapDumpBuilder;

        for (auto* jsContext : jsContexts) {
            auto heapDump = dynamic_cast<HermesJavaScriptContext*>(jsContext)->dumpHeap(exceptionTracker);
            if (!exceptionTracker) {
                return BytesView();
            }

            heapDumpBuilder.appendDump(heapDump.data(), heapDump.size(), exceptionTracker);
            if (!exceptionTracker) {
                return BytesView();
            }
        }

        return heapDumpBuilder.build();
    } else {
        exceptionTracker.onError("Heap dump support not enabled");
        return BytesView();
    }
}

} // namespace Valdi::Hermes
