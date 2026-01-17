//
//  AndroidValueMarshallerRegistry.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/14/23.
//

#pragma once

#include "valdi/android/JavaValueDelegate.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/ValueMarshallerRegistry.hpp"
#include "valdi_core/jni/JavaValue.hpp"

namespace Valdi {
class ValueSchemaRegistry;
class Marshaller;
class ValueSchema;
} // namespace Valdi

namespace ValdiAndroid {

class JavaClassDelegate;
class JavaObjectClassDelegate;
class JavaInterfaceClassDelegate;
class JavaEnumClassDelegate;
class JavaUntypedClassDelegate;
class JavaClass;
class ValueSchemaRegistryListenerImpl;

class FieldNameMap {
public:
    FieldNameMap();
    ~FieldNameMap();

    enum class FieldType {
        Class,
        InterfaceMethod,
        InterfaceProperty,
        EnumCaseValue,
    };

    Valdi::StringBox getFieldName(size_t fieldIndex, const Valdi::StringBox& propertyName, FieldType fieldType) const;

    void appendFieldName(size_t fieldIndex, const Valdi::StringBox& fieldName);

private:
    Valdi::FlatMap<size_t, Valdi::StringBox> _map;
};

/**
 Android Integration of the ValueSchemaRegistry and ValueMarshallerRegistry.
 Allows marshalling and unmarshalling of Java objects as ValueTypedObject and
 ValueTypedProxyObject instances.
 */
class AndroidValueMarshallerRegistry : public Valdi::SimpleRefCountable,
                                       public JavaTypesResolver,
                                       public Valdi::ValueMarshallerRegistryListener {
public:
    AndroidValueMarshallerRegistry();
    ~AndroidValueMarshallerRegistry() override;

    Valdi::Result<Valdi::Value> marshallObject(const Valdi::StringBox& className, const JavaValue& object);

    Valdi::Result<JavaValue> unmarshallObject(const Valdi::StringBox& className, const Valdi::Value& value);

    Valdi::Result<Valdi::Void> setActiveSchemaInMarshaller(const Valdi::StringBox& className,
                                                           Valdi::Marshaller& marshaller);

    Valdi::Result<Valdi::Value> getEnumValue(const Valdi::StringBox& className, const JavaValue& enumValue);

    Valdi::Ref<Valdi::PlatformObjectClassDelegate<JavaValue>> getObjectClassDelegateForName(
        const Valdi::StringBox& className) final;

    Valdi::Ref<Valdi::PlatformObjectClassDelegate<JavaValue>> getInterfaceClassDelegateForName(
        const Valdi::StringBox& className) final;

    Valdi::Ref<Valdi::PlatformEnumClassDelegate<JavaValue>> getEnumClassDelegateForName(
        const Valdi::StringBox& className) final;

    Valdi::ValueSchema getSchemaForInterfacePropertyUnmarshaller(const Valdi::ValueSchema& schema) final;

private:
    Valdi::Ref<Valdi::ValueSchemaRegistry> _schemaRegistry;
    Valdi::ValueMarshallerRegistry<JavaValue> _valueMarshallerRegistry;
    Valdi::FlatMap<Valdi::StringBox, Valdi::Ref<RefCountable>> _registeredClassByName;

    struct RegisteredSchema {
        Valdi::ValueSchemaRegistrySchemaIdentifier identifier;
        Valdi::ValueSchema schema;

        inline RegisteredSchema(Valdi::ValueSchemaRegistrySchemaIdentifier identifier, const Valdi::ValueSchema& schema)
            : identifier(identifier), schema(schema) {}
    };

    friend ValueSchemaRegistryListenerImpl;

    Valdi::Result<RegisteredSchema> parseAndRegisterSchema(const std::string& prefix,
                                                           const std::string_view& suffix,
                                                           const JavaValue& typeReferences);

    Valdi::ValueSchemaRegistrySchemaIdentifier registerUntyped(const Valdi::StringBox& className);

    Valdi::Result<JavaClassDelegate*> getLoadedClassDelegateForClassName(const Valdi::StringBox& className);

    Valdi::Result<JavaClassDelegate*> getOrCreateRegisteredMarshallableClass(const Valdi::StringBox& className);

    static FieldNameMap toFieldMap(const JavaValue& propertyReplacements);

    Valdi::Result<JavaClassDelegate*> registerObjectClassDelegate(const Valdi::StringBox& className,
                                                                  const JavaClass& javaClass,
                                                                  const std::string_view& schemaSuffix,
                                                                  const FieldNameMap& fieldMap,
                                                                  const JavaValue& typeReferences);

    Valdi::Result<JavaClassDelegate*> registerInterfaceClassDelegate(const Valdi::StringBox& className,
                                                                     const JavaClass& javaClass,
                                                                     const std::string_view& schemaSuffix,
                                                                     const FieldNameMap& fieldMap,
                                                                     const JavaValue& proxyClass,
                                                                     const JavaValue& typeReferences);

    Valdi::Result<JavaClassDelegate*> registerStringEnumClassDelegate(const Valdi::StringBox& className,
                                                                      const JavaClass& javaClass,
                                                                      const std::string_view& schemaSuffix,
                                                                      const FieldNameMap& fieldMap);

    Valdi::Result<JavaClassDelegate*> registerIntEnumClassDelegate(const Valdi::StringBox& className,
                                                                   const JavaClass& javaClass,
                                                                   const std::string_view& schemaSuffix,
                                                                   const FieldNameMap& fieldMap);

    Valdi::Result<JavaClassDelegate*> registerEnumClassDelegate(char enumType,
                                                                const Valdi::StringBox& className,
                                                                const JavaClass& javaClass,
                                                                const std::string_view& schemaSuffix,
                                                                const FieldNameMap& fieldMap);

    Valdi::Result<JavaClassDelegate*> registerUntypedClassDelegate(const Valdi::StringBox& className,
                                                                   const JavaClass& javaClass);
};

} // namespace ValdiAndroid
