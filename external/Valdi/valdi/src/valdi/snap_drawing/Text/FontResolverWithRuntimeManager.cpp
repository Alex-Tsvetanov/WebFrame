#include "valdi/snap_drawing/Text/FontResolverWithRuntimeManager.hpp"
#include "valdi/runtime/RuntimeManager.hpp"

namespace snap::drawing {

FontResolverWithRuntimeManager::FontResolverWithRuntimeManager(const Valdi::Ref<Valdi::RuntimeManager>& runtimeManager)
    : _runtimeManager(runtimeManager.toWeak()) {}
FontResolverWithRuntimeManager::~FontResolverWithRuntimeManager() = default;

Valdi::Ref<snap::drawing::LoadableTypeface> FontResolverWithRuntimeManager::resolveTypefaceWithName(
    const Valdi::StringBox& name) {
    auto separatorIndex = name.indexOf(':');
    if (!separatorIndex) {
        // Not a module font
        return nullptr;
    }

    auto runtimeManager = _runtimeManager.lock();
    if (runtimeManager == nullptr) {
        return nullptr;
    }

    auto moduleName = name.substring(0, separatorIndex.value());
    auto path = STRING_FORMAT("res/{}.ttf", name.substring(separatorIndex.value() + 1));

    for (const auto& runtime : runtimeManager->getAllRuntimes()) {
        auto bundle = runtime->getResourceManager().getBundle(moduleName);
        auto fontData = bundle->getEntry(path);
        if (fontData) {
            return snap::drawing::LoadableTypeface::fromBytes(name, fontData.value());
        }
    }

    return nullptr;
}

} // namespace snap::drawing
