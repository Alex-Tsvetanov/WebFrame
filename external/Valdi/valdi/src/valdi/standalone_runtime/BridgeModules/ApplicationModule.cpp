#include "valdi/standalone_runtime/BridgeModules/ApplicationModule.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueArray.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace Valdi {

#define BIND_METHOD(module, __method__)                                                                                \
    module.setMapValue(                                                                                                \
        STRINGIFY(__method__),                                                                                         \
        Valdi::Value(Valdi::ValueFunctionWithCallable::makeFromMethod(this, &ApplicationModule::__method__)))

ApplicationModule::ApplicationModule() = default;

ApplicationModule::~ApplicationModule() = default;

Valdi::StringBox ApplicationModule::getModulePath() {
    return STRING_LITERAL("valdi_core/src/ApplicationBridge");
}

Valdi::Value ApplicationModule::loadModule() {
    Valdi::Value module;
    BIND_METHOD(module, observeEnteredBackground);
    BIND_METHOD(module, observeEnteredForeground);
    BIND_METHOD(module, observeKeyboardHeight);
    return module;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value ApplicationModule::observeEnteredBackground() {
    return observeNoOp();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value ApplicationModule::observeEnteredForeground() {
    return observeNoOp();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value ApplicationModule::observeKeyboardHeight() {
    return observeNoOp();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value ApplicationModule::observeNoOp() {
    Valdi::Value out;
    out.setMapValue(
        "cancel",
        Valdi::Value(Valdi::makeShared<Valdi::ValueFunctionWithCallable>(
            [](const Valdi::ValueFunctionCallContext& /*callContext*/) -> Valdi::Value { return Valdi::Value(); })));
    return out;
}

} // namespace Valdi
