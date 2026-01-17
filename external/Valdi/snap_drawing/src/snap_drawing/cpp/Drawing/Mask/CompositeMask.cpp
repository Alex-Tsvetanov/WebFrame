#include "snap_drawing/cpp/Drawing/Mask/CompositeMask.hpp"

#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace snap::drawing {

CompositeMask::CompositeMask(std::vector<Valdi::Ref<IMask>>&& masks) : _masks(std::move(masks)) {
    for (const auto& mask : _masks) {
        _bounds.join(mask->getBounds());
    }
}

CompositeMask::~CompositeMask() = default;

Rect CompositeMask::getBounds() const {
    return _bounds;
}

void CompositeMask::prepare(SkCanvas* canvas) {
    for (const auto& mask : _masks) {
        mask->prepare(canvas);
    }
}

void CompositeMask::apply(SkCanvas* canvas) {
    // Need to reverse the order of the `prepare` operations because the flow is stack based.
    for (size_t i = _masks.size(); i-- > 0;) {
        const auto& mask = _masks[i];
        mask->apply(canvas);
    }
}

String CompositeMask::getDescription() {
    String output = STRING_FORMAT("Composite bounds is {}\n", _bounds);
    for (const auto& mask : _masks) {
        output += STRING_FORMAT(" - {}\n", mask->getDescription());
    }
    return output;
}

} // namespace snap::drawing
