#pragma once

#include "valdi_core/SCValdiValueUtils.h"
#include "valdi_core/bindings/UndefExceptions_objc.hpp"

namespace ValdiIOS {

struct InternedStringTranslator {
    using CppType = Valdi::StringBox;
    using ObjcType = NSString*;

    static CppType toCpp(ObjcType data) {
        return InternedStringFromNSString(data);
    }

    static ObjcType fromCpp(const CppType& c) {
        return NSStringFromString(c);
    }

    using Boxed = InternedStringTranslator;
};

} // namespace ValdiIOS
