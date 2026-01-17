//
//  SnapDrawingModuleFactoriesProvider.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/16/24.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/ModuleFactoriesProvider.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Lazy.hpp"

namespace Valdi {
class ViewManagerContext;
}

namespace snap::drawing {

class Runtime;

class SnapDrawingModuleFactoriesProvider : public Valdi::SharedPtrRefCountable,
                                           public snap::valdi_core::ModuleFactoriesProvider {
public:
    SnapDrawingModuleFactoriesProvider(Valdi::Function<Ref<Runtime>()> runtimeProvider);
    ~SnapDrawingModuleFactoriesProvider() override;

    std::vector<std::shared_ptr<snap::valdi_core::ModuleFactory>> createModuleFactories(
        const Valdi::Value& runtimeager) override;

private:
    Valdi::Function<Ref<Runtime>()> _runtimeProvider;
};

} // namespace snap::drawing
