//
//  JSPropertyNameIndex.cpp
//  valdi
//
//  Created by Simon Corsin on 5/14/21.
//

#include "valdi/runtime/JavaScript/JSPropertyNameIndex.hpp"

namespace Valdi {

void loadPropertyNames(IJavaScriptContext* context,
                       size_t size,
                       std::string_view* cppPropertyNames,
                       JSPropertyName* outJsPropertyNames) {
    if (context == nullptr) {
        return;
    }

    for (size_t i = 0; i < size; i++) {
        auto propertyNameRef = context->newPropertyName(cppPropertyNames[i]);
        outJsPropertyNames[i] = propertyNameRef.unsafeReleaseValue();
    }
}

void unloadPropertyNames(IJavaScriptContext* context, size_t size, JSPropertyName* jsPropertyNames) {
    if (context == nullptr) {
        return;
    }

    for (size_t i = 0; i < size; i++) {
        auto jsPropertyName = jsPropertyNames[i];
        context->releasePropertyName(jsPropertyName);
    }
}

} // namespace Valdi
