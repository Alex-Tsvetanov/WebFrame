//
//  JavaScriptValueMarshaller.cpp
//  valdi
//
//  Created by Simon Corsin on 2/2/23.
//

#include "valdi/runtime/JavaScript/JavaScriptValueMarshaller.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"
#include "valdi/runtime/JavaScript/JavaScriptValueDelegate.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/PlatformObjectAttachments.hpp"
#include "valdi_core/cpp/Utils/PlatformValueDelegate.hpp"
#include "valdi_core/cpp/Utils/ValueMarshallerRegistry.hpp"
#include "valdi_core/cpp/Utils/ValueTypedObject.hpp"
#include "valdi_core/cpp/Utils/ValueTypedProxyObject.hpp"

namespace Valdi {

class JavaScriptValueTypeConverter : public ValueMarshallerProcessor<JSValueRef> {
public:
    JavaScriptValueTypeConverter(IJavaScriptContext& jsContext, JSValueRef preprocess, JSValueRef postprocess)
        : _jsContext(jsContext),
          _preprocess(jsContext.ensureRetainedValue(std::move(preprocess))),
          _postprocess(jsContext.ensureRetainedValue(std::move(postprocess))) {}
    ~JavaScriptValueTypeConverter() override = default;

    JSValueRef preprocess(const JSValueRef* receiver,
                          const JSValueRef& value,
                          const ReferenceInfoBuilder& /*referenceInfoBuilder*/,
                          ExceptionTracker& exceptionTracker) final {
        JSFunctionCallContext callContext(_jsContext, &value, 1, dynamic_cast<JSExceptionTracker&>(exceptionTracker));

        return _jsContext.callObjectAsFunction(_preprocess.get(), callContext);
    }

    JSValueRef postprocess(const JSValueRef& value,
                           const ReferenceInfoBuilder& /*referenceInfoBuilder*/,
                           ExceptionTracker& exceptionTracker) final {
        JSFunctionCallContext callContext(_jsContext, &value, 1, dynamic_cast<JSExceptionTracker&>(exceptionTracker));
        return _jsContext.callObjectAsFunction(_postprocess.get(), callContext);
    }

private:
    IJavaScriptContext& _jsContext;
    JSValueRef _preprocess;
    JSValueRef _postprocess;
};

JavaScriptValueMarshaller::JavaScriptValueMarshaller(IJavaScriptContext& jsContext,
                                                     bool disableProxyObjectStore,
                                                     JSExceptionTracker& exceptionTracker)
    : _jsContext(jsContext),
      _valueDelegate(makeShared<JavaScriptValueDelegate>(jsContext, disableProxyObjectStore, exceptionTracker)),
      _marshallerRegistry(ValueSchemaTypeResolver(nullptr), _valueDelegate, nullptr, STRING_LITERAL("Js")) {}

JavaScriptValueMarshaller::~JavaScriptValueMarshaller() {
    _valueDelegate->teardown();
}

JSValueRef JavaScriptValueMarshaller::unmarshallTypedObject(const Ref<ValueTypedObject>& typedObject,
                                                            const ReferenceInfoBuilder& referenceInfoBuilder,
                                                            JSExceptionTracker& exceptionTracker) {
    return unmarshallValueOfClassSchema(
        typedObject->getSchema(), Value(typedObject), referenceInfoBuilder, exceptionTracker);
}

JSValueRef JavaScriptValueMarshaller::unmarshallProxyObject(const Ref<ValueTypedProxyObject>& proxyObject,
                                                            const ReferenceInfoBuilder& referenceInfoBuilder,
                                                            JSExceptionTracker& exceptionTracker) {
    return unmarshallValueOfClassSchema(
        proxyObject->getTypedObject()->getSchema(), Value(proxyObject), referenceInfoBuilder, exceptionTracker);
}

JSValueRef JavaScriptValueMarshaller::unmarshallValueOfClassSchema(const Ref<ClassSchema>& classSchema,
                                                                   Value&& value,
                                                                   const ReferenceInfoBuilder& referenceInfoBuilder,
                                                                   JSExceptionTracker& exceptionTracker) {
    auto valueMarshaller = getValueMarshallerForClassSchema(classSchema, exceptionTracker);
    if (valueMarshaller == nullptr) {
        return _jsContext.newUndefined();
    }

    return valueMarshaller->unmarshall(Value(value), referenceInfoBuilder, exceptionTracker);
}

Value JavaScriptValueMarshaller::marshall(const JSValueRef& jsValue,
                                          const ValueSchema& valueSchema,
                                          const ReferenceInfoBuilder& referenceInfoBuilder,
                                          JSExceptionTracker& exceptionTracker) {
    Ref<ValueMarshaller<JSValueRef>> valueMarshaller;

    if (valueSchema.isClass()) {
        // Leverage cache for classes
        valueMarshaller = getValueMarshallerForClassSchema(valueSchema.getClassRef(), exceptionTracker);
    } else {
        auto valueMarshallerWithSchema = _marshallerRegistry.getValueMarshaller(valueSchema, exceptionTracker);
        valueMarshaller = std::move(valueMarshallerWithSchema.valueMarshaller);
    }

    if (valueMarshaller == nullptr) {
        return Value::undefined();
    }

    return valueMarshaller->marshall(nullptr, jsValue, referenceInfoBuilder, exceptionTracker);
}

void JavaScriptValueMarshaller::registerTypeConverter(const StringBox& typeName,
                                                      const JSValueRef& converter,
                                                      JSExceptionTracker& exceptionTracker) {
    auto preprocess = _jsContext.getObjectProperty(converter.get(), "toIntermediate", exceptionTracker);
    if (!exceptionTracker) {
        return;
    }

    auto postprocess = _jsContext.getObjectProperty(converter.get(), "toTypeScript", exceptionTracker);
    if (!exceptionTracker) {
        return;
    }

    auto typeConverter =
        makeShared<JavaScriptValueTypeConverter>(_jsContext, std::move(preprocess), std::move(postprocess));
    auto schema = ValueSchema::typeReference(
        ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHintConverted, typeName));

    _marshallerRegistry.registerProcessor(ValueSchemaRegistryKey(std::move(schema)), typeConverter);
}

Ref<ValueMarshaller<JSValueRef>> JavaScriptValueMarshaller::getValueMarshallerForClassSchema(
    const Ref<ClassSchema>& classSchema, JSExceptionTracker& exceptionTracker) {
    const auto& it = _valueMarshallerByClass.find(classSchema.get());
    if (it != _valueMarshallerByClass.end()) {
        return it->second;
    }

    auto valueMarshallerWithSchema =
        _marshallerRegistry.getValueMarshaller(ValueSchema::cls(classSchema), exceptionTracker);
    if (!exceptionTracker) {
        return nullptr;
    }

    _valueMarshallerByClass[classSchema.get()] = valueMarshallerWithSchema.valueMarshaller;
    _classes.emplace_back(classSchema);

    return valueMarshallerWithSchema.valueMarshaller;
}

static Value unwrapSingleProxy(const PlatformObjectAttachments& objectAttachments,
                               JSExceptionTracker& exceptionTracker) {
    auto proxies = objectAttachments.getAllProxies();
    if (proxies.empty()) {
        return Value::undefined();
    }

    if (proxies.size() == 1) {
        return Value(proxies[0]);
    }

    std::vector<StringBox> classNames;
    for (const auto& proxy : proxies) {
        classNames.emplace_back(proxy->getTypedObject()->getClassName());
    }

    exceptionTracker.onError(Error(STRING_FORMAT("Cannot unwrap proxy: object contains multiple proxies [{}]",
                                                 StringBox::join(classNames, ", "))));

    return Value::undefined();
}

Value JavaScriptValueMarshaller::tryUnwrap(const JSValue& jsValue, JSExceptionTracker& exceptionTracker) {
    auto& jsObjectStore = _valueDelegate->getJsObjectStore();
    if (VALDI_LIKELY(!jsObjectStore.hasValueForObjectKey(jsValue))) {
        return Value::undefined();
    }

    auto attachedValue = jsObjectStore.getValueForObjectKey(jsValue, exceptionTracker);
    if (attachedValue == nullptr) {
        return Value::undefined();
    }

    auto objectAttachments = castOrNull<PlatformObjectAttachments>(attachedValue);
    if (objectAttachments == nullptr) {
        return Value::undefined();
    }

    return unwrapSingleProxy(*objectAttachments, exceptionTracker);
}

} // namespace Valdi
