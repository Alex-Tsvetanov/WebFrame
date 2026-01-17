#pragma once

#include "valdi_core/SCValdiObjCConversionUtils.h"
#include "valdi_core/bindings/UndefExceptions_objc.hpp"

namespace ValdiIOS {

struct ValueTranslator {
    using CppType = Valdi::Value;
    using ObjcType = id;

    static CppType toCpp(ObjcType data) {
        return ValueFromNSObject(data);
    }

    static ObjcType fromCpp(const CppType& c) {
        return NSObjectFromValue(c);
    }

    using Boxed = id;
};

} // namespace ValdiIOS
