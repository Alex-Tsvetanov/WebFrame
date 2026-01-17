//
//  HermesUtils.cpp
//  valdi-hermes
//
//  Created by Simon Corsin on 9/27/23.
//

#include "valdi/hermes/HermesUtils.hpp"
#include "valdi/hermes/HermesJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"
#include "valdi_core/cpp/Constants.hpp"

namespace Valdi::Hermes {

JSFunctionData::JSFunctionData(HermesJavaScriptContext& jsContext, const Valdi::Ref<Valdi::JSFunction>& function)
    : jsContext(jsContext), function(function) {}
JSFunctionData::~JSFunctionData() = default;

static hermes::vm::ExecutionStatus onJsCallError(hermes::vm::Runtime& runtime,
                                                 Valdi::JSExceptionTracker& exceptionTracker) {
    auto exception = exceptionTracker.getExceptionAndClear();
    return runtime.setThrownValue(HermesJavaScriptContext::toHermesValue(exception.get()));
}

hermes::vm::CallResult<hermes::vm::HermesValue> callTrampoline(void* context,
                                                               hermes::vm::Runtime& runtime,
                                                               hermes::vm::NativeArgs args) {
    auto* functionData = unsafeBridgeUnretained<JSFunctionData>(context);

    auto thisRef = functionData->jsContext.toJSValueRef(args.getThisArg());
    JSValueRef outArguments[args.getArgCount()];
    for (size_t i = 0; i < args.getArgCount(); i++) {
        outArguments[i] = functionData->jsContext.toJSValueRef(args.getArg(i));
    }

    JSExceptionTracker exceptionTracker(functionData->jsContext);
    JSFunctionNativeCallContext callContext(functionData->jsContext,
                                            &outArguments[0],
                                            args.getArgCount(),
                                            exceptionTracker,
                                            functionData->function->getReferenceInfo());
    callContext.setThisValue(thisRef.get());

    if (functionData->jsContext.interruptRequested()) {
        functionData->jsContext.onInterrupt();
    }

    auto result = (*functionData->function)(callContext);

    if (VALDI_LIKELY(exceptionTracker)) {
        return HermesJavaScriptContext::toHermesValue(result.get());
    } else {
        return onJsCallError(runtime, exceptionTracker);
    }
}

void ByteBufferOStream::write_impl(const char* Ptr, size_t Size) {
    _output.append(Ptr, Ptr + Size);
}

uint64_t ByteBufferOStream::current_pos() const {
    return static_cast<uint64_t>(_output.size());
}

ByteBufferOStream::ByteBufferOStream(ByteBuffer& output) : _output(output) {}
ByteBufferOStream::~ByteBufferOStream() = default;

BytesView ByteBufferOStream::toBytesView() {
    flush();
    return _output.toBytesView();
}

} // namespace Valdi::Hermes
