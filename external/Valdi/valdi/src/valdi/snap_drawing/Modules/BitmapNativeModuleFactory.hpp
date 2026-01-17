//
//  BitmapNativeModuleFactory.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/17/24.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/ModuleFactory.hpp"

namespace Valdi {
class ValueFunctionCallContext;
} // namespace Valdi

namespace snap::drawing {

class BitmapNativeModuleFactory final : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    BitmapNativeModuleFactory();
    ~BitmapNativeModuleFactory() final;

    Valdi::StringBox getModulePath() final;
    Valdi::Value loadModule() final;

private:
    Valdi::Value createNativeBitmap(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value decodeNativeBitmap(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value disposeNativeBitmap(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value encodeNativeBitmap(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value lockNativeBitmap(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value unlockNativeBitmap(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value getNativeBitmapInfo(const Valdi::ValueFunctionCallContext& callContext);
};

} // namespace snap::drawing
