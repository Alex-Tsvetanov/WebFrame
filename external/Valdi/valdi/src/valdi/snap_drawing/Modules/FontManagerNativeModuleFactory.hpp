//
//  FontManagerNativeModuleFactory.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/16/24.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/ModuleFactory.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"

namespace Valdi {
class ValueFunctionCallContext;
}

namespace snap::drawing {

class FontManager;
class Runtime;

class FontManagerNativeModuleFactory final : public Valdi::SharedPtrRefCountable,
                                             public snap::valdi_core::ModuleFactory {
public:
    FontManagerNativeModuleFactory(Valdi::Function<Ref<Runtime>()> runtimeProvider);
    ~FontManagerNativeModuleFactory() final;

    Valdi::StringBox getModulePath() final;
    Valdi::Value loadModule() final;

private:
    Valdi::Function<Ref<Runtime>()> _runtimeProvider;

    Valdi::Value getDefaultFontManager(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value makeScopedFontManager(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value registerFontFromData(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value registerFontFromFilePath(const Valdi::ValueFunctionCallContext& callContext);
};

} // namespace snap::drawing
