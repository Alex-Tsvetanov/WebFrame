#include "valdi/jscore/JavaScriptCoreContextFactory.hpp"
#include "valdi/jscore/JavaScriptCoreContext.hpp"

namespace ValdiJSCore {

JavaScriptCoreContextFactory::JavaScriptCoreContextFactory() = default;

const char* JavaScriptCoreContextFactory::getName() {
    return "JavaScriptCore";
}

bool JavaScriptCoreContextFactory::requiresDedicatedThread() {
    return false;
}

Valdi::Ref<Valdi::IJavaScriptContext> JavaScriptCoreContextFactory::createJsContext(
    Valdi::JavaScriptTaskScheduler* taskScheduler, Valdi::ILogger& logger) {
    return Valdi::makeShared<JavaScriptCoreContext>(taskScheduler, logger);
}

Valdi::BytesView JavaScriptCoreContextFactory::dumpHeap(std::span<Valdi::IJavaScriptContext*> /*jsContexts*/,
                                                        Valdi::JSExceptionTracker& exceptionTracker) {
    exceptionTracker.onError("JavaScriptCore does not support heap dumps");
    return Valdi::BytesView();
}

} // namespace ValdiJSCore
