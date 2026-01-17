//
//  JavaDjinniUtils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 16/06/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace Valdi {

template<typename ToCpp>
auto djinniObjectFromJavaObjectInValue(ToCpp&& toCpp, const Value& value) -> decltype(toCpp(nullptr, nullptr)) {
    auto globalRefJavaObject = value.getTypedRef<ValdiAndroid::GlobalRefJavaObject>();
    if (globalRefJavaObject == nullptr) {
        return nullptr;
    }

    auto ref = globalRefJavaObject->get();

    return toCpp(ValdiAndroid::JavaEnv::getUnsafeEnv(), ref.get());
}

} // namespace Valdi
