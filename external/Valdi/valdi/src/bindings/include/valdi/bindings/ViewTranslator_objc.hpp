#pragma once

#include "valdi/ios/CPPBindings/UIViewHolder.h"

namespace ValdiIOS {

struct ViewTranslator {
    using CppType = Valdi::Ref<Valdi::View>;
    using ObjcType = UIView*;

    static ObjcType fromCpp(const CppType& c) {
        return UIViewHolder::uiViewFromRef(c);
    }

    using Boxed = Valdi::Ref<Valdi::View>;
};

} // namespace ValdiIOS
