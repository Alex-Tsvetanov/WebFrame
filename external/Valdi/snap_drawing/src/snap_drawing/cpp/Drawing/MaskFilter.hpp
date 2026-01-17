#pragma once

#include "snap_drawing/cpp/Utils/Scalar.hpp"
#include "snap_drawing/cpp/Utils/SkiaBridge.hpp"

#include "include/core/SkBlurTypes.h"
#include "include/core/SkMaskFilter.h"

namespace snap::drawing {

enum BlurStyle {
    BlurStyleNormal = kNormal_SkBlurStyle, //!< fuzzy inside and outside
    BlurStyleSolid = kSolid_SkBlurStyle,   //!< solid inside, fuzzy outside
    BlurStyleOuter = kOuter_SkBlurStyle,   //!< nothing inside, fuzzy outside
    BlurStyleInner = kInner_SkBlurStyle,   //!< fuzzy inside, nothing outside
};

class MaskFilter : public WrappedSkValue<sk_sp<SkMaskFilter>> {
public:
    MaskFilter();
    ~MaskFilter();

    static MaskFilter makeBlur(BlurStyle blurStyle, Scalar sigma);

private:
    explicit MaskFilter(sk_sp<SkMaskFilter> skValue);
};

} // namespace snap::drawing