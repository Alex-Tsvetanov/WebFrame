//
//  IJavaScriptBridge.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include <span>

namespace Valdi {

class ILogger;

struct IJavaScriptBridgeConfig {};

class IJavaScriptBridge {
public:
    IJavaScriptBridge() = default;
    virtual ~IJavaScriptBridge() = default;

    virtual void configure(const IJavaScriptBridgeConfig& config) {}

    virtual const char* getName() = 0;

    /**
     Whether the JS engine requires to be called from the same exact thread id.
     The engine is already guaranteed that it will never be called concurrently,
     but it might be called from different threads because of GCD.
     If this flag is true, every calls into the engine will always come from the
     same thread.
     */
    virtual bool requiresDedicatedThread() {
        return false;
    }

    /**
     Create the JavaScriptContext.
     The provided DispatchQueue is where all calls to the
     IJavaScriptContext and JSValues will happen.
     */
    virtual Valdi::Ref<IJavaScriptContext> createJsContext(JavaScriptTaskScheduler* taskScheduler, ILogger& logger) = 0;

    /**
     Dumps the heap for the given JSContexts.
     */
    virtual BytesView dumpHeap(std::span<IJavaScriptContext*> jsContexts, JSExceptionTracker& exceptionTracker) = 0;

    /**
     * Starts the JS debugger server for the created JSContexts.
     */
    virtual void startJsDebuggerServer([[maybe_unused]] ILogger& logger) {
        return;
    }

    /**
     * Stops the JS debugger server.
     */
    virtual void stopJsDebuggerServer() {
        return;
    }
};

} // namespace Valdi
