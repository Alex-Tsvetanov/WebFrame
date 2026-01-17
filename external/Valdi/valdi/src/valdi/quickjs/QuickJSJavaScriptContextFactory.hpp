//
//  QuickJSJavaScriptContextFactory.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/19/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptBridge.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"

namespace ValdiQuickJS {

class QuickJSJavaScriptContextFactory : public Valdi::IJavaScriptBridge {
public:
    QuickJSJavaScriptContextFactory();

    const char* getName() override;

    bool requiresDedicatedThread() override;

    Valdi::Ref<Valdi::IJavaScriptContext> createJsContext(Valdi::JavaScriptTaskScheduler* taskScheduler,
                                                          Valdi::ILogger& logger) override;

    Valdi::BytesView dumpHeap(std::span<Valdi::IJavaScriptContext*> jsContexts,
                              Valdi::JSExceptionTracker& exceptionTracker) final;
};

} // namespace ValdiQuickJS
