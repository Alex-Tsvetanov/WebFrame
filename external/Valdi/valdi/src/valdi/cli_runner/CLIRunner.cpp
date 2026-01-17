#include "valdi/jsbridge/JavaScriptBridge.hpp"
#include "valdi/standalone_runtime/Arguments.hpp"
#include "valdi/standalone_runtime/SignalHandler.hpp"
#include "valdi/standalone_runtime/ValdiStandaloneMain.hpp"

#include "utils/platform/BuildOptions.hpp"

namespace Valdi {

int valdiCLIRun(const char* scriptPath, int argc, const char** argv) {
    SignalHandler::install();

    StandaloneArguments standaloneArguments;
    standaloneArguments.scriptPath = StringBox::fromCString(scriptPath);
    standaloneArguments.enableHotReloader = false;

    if constexpr (snap::kIsDevBuild) {
        standaloneArguments.enableDebuggerService = true;
        standaloneArguments.logLevel = LogTypeDebug;
    } else {
        standaloneArguments.enableDebuggerService = false;
        standaloneArguments.logLevel = LogTypeError;
    }

    Arguments arguments(argc, argv);
    while (arguments.hasNext()) {
        standaloneArguments.jsArguments.emplace_back(arguments.next());
    }
    standaloneArguments.jsBridge = JavaScriptBridge::get(snap::valdi_core::JavaScriptEngineType::Auto);

    return runValdiStandalone(standaloneArguments);
}

} // namespace Valdi
