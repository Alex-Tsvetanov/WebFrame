#include "snap_drawing/cpp/Drawing/MaskFilter.hpp"

namespace snap::drawing {

MaskFilter::MaskFilter() : MaskFilter(nullptr) {}

MaskFilter::MaskFilter(sk_sp<SkMaskFilter> skValue) : WrappedSkValue<sk_sp<SkMaskFilter>>(std::move(skValue)) {}
MaskFilter::~MaskFilter() = default;

MaskFilter MaskFilter::makeBlur(BlurStyle blurStyle, Scalar sigma) {
    return MaskFilter(SkMaskFilter::MakeBlur(static_cast<SkBlurStyle>(blurStyle), sigma));
}

} // namespace snap::drawing