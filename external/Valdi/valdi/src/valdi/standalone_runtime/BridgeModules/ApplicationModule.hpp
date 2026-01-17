#pragma once

#include "valdi_core/ModuleFactory.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class ApplicationModule : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    ApplicationModule();
    ~ApplicationModule() override;

    Valdi::StringBox getModulePath() override;
    Valdi::Value loadModule() override;

    Valdi::Value observeEnteredBackground();
    Valdi::Value observeEnteredForeground();
    Valdi::Value observeKeyboardHeight();

private:
    Valdi::Value observeNoOp();
};

} // namespace Valdi
