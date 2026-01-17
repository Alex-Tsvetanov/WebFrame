//
//  JavaScriptValueMarshaller.hpp
//  valdi
//
//  Created by Simon Corsin on 2/2/23.
//

#pragma once

#include "valdi/runtime/JavaScript/JavaScriptTypes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/ValueMarshallerRegistry.hpp"

namespace Valdi {

class IJavaScriptContext;
class ValueTypedProxyObject;
class JavaScriptValueDelegate;

class ClassSchema;
class FunctionSchema;

class JavaScriptValueMarshaller {
public:
    JavaScriptValueMarshaller(IJavaScriptContext& jsContext,
                              bool disableProxyObjectStore,
                              JSExceptionTracker& exceptionTracker);
    ~JavaScriptValueMarshaller();

    JSValueRef unmarshallTypedObject(const Ref<ValueTypedObject>& typedObject,
                                     const ReferenceInfoBuilder& referenceInfoBuilder,
                                     JSExceptionTracker& exceptionTracker);
    JSValueRef unmarshallProxyObject(const Ref<ValueTypedProxyObject>& proxyObject,
                                     const ReferenceInfoBuilder& referenceInfoBuilder,
                                     JSExceptionTracker& exceptionTracker);

    Value marshall(const JSValueRef& jsValue,
                   const ValueSchema& valueSchema,
                   const ReferenceInfoBuilder& referenceInfoBuilder,
                   JSExceptionTracker& exceptionTracker);

    /**
     Attempt to unwrap an attached instance from the given JSValue.
     Will return undefined if the unwrapping attempt was succesfully
     performed but nothing was found. Will throw if the unwrapping fails
     */
    Value tryUnwrap(const JSValue& jsValue, JSExceptionTracker& exceptionTracker);

    void registerTypeConverter(const StringBox& typeName,
                               const JSValueRef& converter,
                               JSExceptionTracker& exceptionTracker);

private:
    IJavaScriptContext& _jsContext;
    Ref<JavaScriptValueDelegate> _valueDelegate;
    ValueMarshallerRegistry<JSValueRef> _marshallerRegistry;
    FlatMap<RefCountable*, Ref<ValueMarshaller<JSValueRef>>> _valueMarshallerByClass;
    // Used to retain the classes, to ensure their pointers stay stable
    // so that we can rely on the pointer as a key. In practice, ClassSchema and FunctionSchema
    // in the app never changes once they reach JS, as they are fully resolved schema representing
    // models used in the app.
    std::vector<Ref<RefCountable>> _classes;

    Ref<ValueMarshaller<JSValueRef>> getValueMarshallerForClassSchema(const Ref<ClassSchema>& classSchema,
                                                                      JSExceptionTracker& exceptionTracker);
    Ref<ValueMarshaller<JSValueRef>> getValueMarshallerForFunctionSchema(const Ref<FunctionSchema>& functionSchema,
                                                                         JSExceptionTracker& exceptionTracker);

    JSValueRef unmarshallValueOfClassSchema(const Ref<ClassSchema>& classSchema,
                                            Value&& value,
                                            const ReferenceInfoBuilder& referenceInfoBuilder,
                                            JSExceptionTracker& exceptionTracker);
};

} // namespace Valdi
