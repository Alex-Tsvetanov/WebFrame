#pragma once

#include "valdi_core/SCValdiObjCConversionUtils.h"
#include "valdi_core/bindings/UndefExceptions_objc.hpp"

namespace ValdiIOS {

struct BytesTranslator {
    using CppType = Valdi::BytesView;
    using ObjcType = NSData*;

    static CppType toCpp(ObjcType data) {
        return BufferFromNSData(data);
    }

    static ObjcType fromCpp(const CppType& c) {
        return NSDataFromBuffer(c);
    }

    using Boxed = BytesTranslator;
};

} // namespace ValdiIOS
