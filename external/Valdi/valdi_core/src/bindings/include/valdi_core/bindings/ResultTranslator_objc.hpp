#pragma once

#include "valdi_core/SCValdiResult+CPP.h"
#include "valdi_core/cpp/Utils/PlatformResult.hpp"

namespace ValdiIOS {

struct ResultTranslator {
    using CppType = Valdi::PlatformResult;
    using ObjcType = SCValdiResult*;

    static CppType toCpp(ObjcType data) {
        return SCValdiResultToPlatformResult(data);
    }

    static ObjcType fromCpp(const CppType& c) {
        return SCValdiResultFromPlatformResult(c);
    }

    using Boxed = SCValdiResult*;
};

} // namespace ValdiIOS
