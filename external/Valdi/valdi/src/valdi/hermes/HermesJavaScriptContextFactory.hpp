//
//  HermesJavaScriptContextFactory.hpp
//  valdi-hermes
//
//  Created by Simon Corsin on 9/27/23.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptBridge.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"

namespace Valdi::Hermes {

class HermesDebuggerServer;

class HermesJavaScriptContextFactory : public IJavaScriptBridge {
public:
    HermesJavaScriptContextFactory();

    const char* getName() override;

    bool requiresDedicatedThread() override;

    Ref<IJavaScriptContext> createJsContext(JavaScriptTaskScheduler* taskScheduler, ILogger& logger) final;

    BytesView dumpHeap(std::span<IJavaScriptContext*> jsContexts, JSExceptionTracker& exceptionTracker) final;

    void startJsDebuggerServer(ILogger& logger) final;

    void stopJsDebuggerServer() final;

private:
#ifdef HERMES_ENABLE_DEBUGGER
    HermesDebuggerServer* _debuggerServer{nullptr};
#endif
};

} // namespace Valdi::Hermes
