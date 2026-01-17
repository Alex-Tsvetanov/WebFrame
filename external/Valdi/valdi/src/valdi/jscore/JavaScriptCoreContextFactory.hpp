#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptBridge.hpp"

namespace ValdiJSCore {

class JavaScriptCoreContextFactory : public Valdi::IJavaScriptBridge {
public:
    JavaScriptCoreContextFactory();

    const char* getName() override;

    bool requiresDedicatedThread() override;

    Valdi::Ref<Valdi::IJavaScriptContext> createJsContext(Valdi::JavaScriptTaskScheduler* taskScheduler,
                                                          Valdi::ILogger& logger) final;

    Valdi::BytesView dumpHeap(std::span<Valdi::IJavaScriptContext*> jsContexts,
                              Valdi::JSExceptionTracker& exceptionTracker) final;
};
} // namespace ValdiJSCore
