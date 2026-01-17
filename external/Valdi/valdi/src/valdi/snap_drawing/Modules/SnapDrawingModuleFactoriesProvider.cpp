//
//  SnapDrawingModuleFactoriesProvider.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/16/24.
//

#include "valdi/snap_drawing/Modules/SnapDrawingModuleFactoriesProvider.hpp"
#include "snap_drawing/cpp/Resources.hpp"
#include "valdi/runtime/Context/ViewManagerContext.hpp"
#include "valdi/runtime/RuntimeManager.hpp"
#include "valdi/snap_drawing/Modules/BitmapNativeModuleFactory.hpp"
#include "valdi/snap_drawing/Modules/FontManagerNativeModuleFactory.hpp"
#include "valdi/snap_drawing/Modules/ManagedContextNativeModuleFactory.hpp"
#include "valdi/snap_drawing/Runtime.hpp"
#include "valdi/snap_drawing/SnapDrawingViewManager.hpp"
#include "valdi_core/cpp/Utils/Lazy.hpp"

namespace snap::drawing {

SnapDrawingModuleFactoriesProvider::SnapDrawingModuleFactoriesProvider(Valdi::Function<Ref<Runtime>()> runtimeProvider)
    : _runtimeProvider(std::move(runtimeProvider)) {}

SnapDrawingModuleFactoriesProvider::~SnapDrawingModuleFactoriesProvider() = default;

std::vector<std::shared_ptr<snap::valdi_core::ModuleFactory>> SnapDrawingModuleFactoriesProvider::createModuleFactories(
    const Valdi::Value& runtimeManager) {
    auto fontManagerModuleFactory = Valdi::makeShared<FontManagerNativeModuleFactory>(_runtimeProvider);

    auto runtimeManagerWeak = runtimeManager.getTypedRef<Valdi::RuntimeManager>().toWeak();
    auto lazy = Valdi::makeShared<Valdi::Lazy<Ref<Valdi::ViewManagerContext>>>(
        [self = Valdi::strongSmallRef(this), runtimeManagerWeak = std::move(runtimeManagerWeak)]() {
            auto runtimeManager = runtimeManagerWeak.lock();
            return runtimeManager->createViewManagerContext(
                *self->_runtimeProvider()->getViewManager(), false, nullptr);
        });

    auto managedContextModuleFactory = Valdi::makeShared<ManagedContextNativeModuleFactory>(
        _runtimeProvider, [lazy]() -> Ref<Valdi::ViewManagerContext> { return lazy->getOrCreate(); });

    auto bitmapModuleFactory = Valdi::makeShared<BitmapNativeModuleFactory>();
    return {
        fontManagerModuleFactory.toShared(), managedContextModuleFactory.toShared(), bitmapModuleFactory.toShared()};
}

} // namespace snap::drawing
