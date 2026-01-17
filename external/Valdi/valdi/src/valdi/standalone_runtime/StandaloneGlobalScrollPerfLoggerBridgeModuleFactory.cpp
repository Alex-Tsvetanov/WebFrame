#include "valdi/standalone_runtime/StandaloneGlobalScrollPerfLoggerBridgeModuleFactory.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace Valdi {

StandaloneGlobalScrollPerfLoggerBridgeModuleFactory::StandaloneGlobalScrollPerfLoggerBridgeModuleFactory() = default;

StringBox StandaloneGlobalScrollPerfLoggerBridgeModuleFactory::getModulePath() {
    return STRING_LITERAL("GlobalScrollPerfLoggerBridgeFactory");
}

Value StandaloneGlobalScrollPerfLoggerBridgeModuleFactory::loadModule() {
    Value module;
    module.setMapValue(
        "createScrollPerfLoggerBridge",
        Value(makeShared<ValueFunctionWithCallable>(
            [](const ValueFunctionCallContext& /*callContext*/) -> Value { return Value::undefined(); })));
    return module;
}
} // namespace Valdi
