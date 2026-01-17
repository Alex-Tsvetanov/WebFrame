//
//  JavaScriptRuntimeDeserializers.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/18/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JSPropertyNameIndex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

template<typename T>
class JavaScriptObjectDeserializer;
class RenderRequest;
class ContextManager;
class JavaScriptStringCache;
struct RenderRequestDescriptor;
class StyleAttributesCache;
class ReferenceInfo;
class ReferenceInfoBuilder;

class JavaScriptRuntimeDeserializers {
public:
    JavaScriptRuntimeDeserializers(IJavaScriptContext& jsContext,
                                   JavaScriptStringCache& stringCache,
                                   StyleAttributesCache& styleAttributesCache);

    ~JavaScriptRuntimeDeserializers();

    Ref<RenderRequest> deserializeRenderRequest(const JSValue& jsValue,
                                                const ReferenceInfo& referenceInfo,
                                                JSExceptionTracker& exceptionTracker);

private:
    IJavaScriptContext& _jsContext;
    JavaScriptStringCache& _stringCache;
    StyleAttributesCache& _styleAttributesCache;

    JSPropertyNameIndex<6> _propertyNames;

    Ref<RenderRequest> parseRequestDescriptor(const RenderRequestDescriptor& requestDescriptor,
                                              JSExceptionTracker& exceptionTracker);

    const Value& getAttachedValue(const RenderRequestDescriptor& requestDescriptor,
                                  const Ref<ValueArray>& values,
                                  size_t index,
                                  const ReferenceInfoBuilder& referenceInfoBuilder,
                                  JSExceptionTracker& exceptionTracker) const;
};

} // namespace Valdi
