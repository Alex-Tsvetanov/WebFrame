#pragma once

#include "snap_drawing/cpp/Text/FontManager.hpp"

namespace Valdi {
class RuntimeManager;
}

namespace snap::drawing {

class FontResolverWithRuntimeManager : public snap::drawing::IFontManagerListener {
public:
    FontResolverWithRuntimeManager(const Valdi::Ref<Valdi::RuntimeManager>& runtimeManager);
    ~FontResolverWithRuntimeManager() override;

    Valdi::Ref<snap::drawing::LoadableTypeface> resolveTypefaceWithName(const Valdi::StringBox& name) override;

private:
    Valdi::Weak<Valdi::RuntimeManager> _runtimeManager;
};

} // namespace snap::drawing
