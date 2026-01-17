//
//  ValueSchema_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 17/01/23
//

#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Schema/ValueSchemaRegistry.hpp"
#include "valdi_core/cpp/Schema/ValueSchemaTypeResolver.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

class TestValueSchemaRegistryListener : public ValueSchemaRegistryListener {
public:
    TestValueSchemaRegistryListener() {}

    ~TestValueSchemaRegistryListener() override = default;

    void setCallback(const ValueSchemaRegistryKey& registryKey, Function<Void()>&& callback) {
        _callbacks[registryKey] = std::move(callback);
    }

    Result<Void> resolveSchemaIdentifierForSchemaKey(ValueSchemaRegistry& registry,
                                                     const ValueSchemaRegistryKey& registryKey) final {
        const auto& it = _callbacks.find(registryKey);
        if (it != _callbacks.end()) {
            return it->second();
        }
        return Error("Cannot find matching register callback");
    }

private:
    FlatMap<ValueSchemaRegistryKey, Function<Void()>> _callbacks;
};

TEST(ValueSchema, supportsPrimitiveTypes) {
    auto schema = ValueSchema::untyped();

    ASSERT_TRUE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_FALSE(schema.isOptional());

    schema = ValueSchema::integer();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isInteger());
    ASSERT_FALSE(schema.isOptional());

    schema = ValueSchema::longInteger();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isLongInteger());
    ASSERT_FALSE(schema.isOptional());

    schema = ValueSchema::doublePrecision();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isDouble());
    ASSERT_FALSE(schema.isOptional());

    schema = ValueSchema::boolean();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isBoolean());
    ASSERT_FALSE(schema.isOptional());

    schema = ValueSchema::string();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isString());
    ASSERT_FALSE(schema.isOptional());
}

TEST(ValueSchema, supportsOptional) {
    auto schema = ValueSchema::untyped().asOptional();

    ASSERT_TRUE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isOptional());

    schema = ValueSchema::integer().asOptional();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isInteger());
    ASSERT_TRUE(schema.isOptional());

    schema = ValueSchema::longInteger().asOptional();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isLongInteger());
    ASSERT_TRUE(schema.isOptional());

    schema = ValueSchema::doublePrecision().asOptional();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isDouble());
    ASSERT_TRUE(schema.isOptional());

    schema = ValueSchema::boolean().asOptional();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isBoolean());
    ASSERT_TRUE(schema.isOptional());

    schema = ValueSchema::string().asOptional();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isString());
    ASSERT_TRUE(schema.isOptional());
}

TEST(ValueSchema, supportsArray) {
    auto schema = ValueSchema::untyped().asArray();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isArray());
    ASSERT_TRUE(schema.getArrayItemSchema().isUntyped());

    schema = ValueSchema::integer().asArray();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isArray());
    ASSERT_TRUE(schema.getArrayItemSchema().isInteger());
    ASSERT_FALSE(schema.getArrayItemSchema().isUntyped());

    schema = ValueSchema::longInteger().asArray();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isArray());
    ASSERT_TRUE(schema.getArrayItemSchema().isLongInteger());
    ASSERT_FALSE(schema.getArrayItemSchema().isUntyped());

    schema = ValueSchema::doublePrecision().asArray();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isArray());
    ASSERT_TRUE(schema.getArrayItemSchema().isDouble());
    ASSERT_FALSE(schema.getArrayItemSchema().isUntyped());

    schema = ValueSchema::boolean().asArray();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isArray());
    ASSERT_TRUE(schema.getArrayItemSchema().isBoolean());
    ASSERT_FALSE(schema.getArrayItemSchema().isUntyped());

    schema = ValueSchema::string().asArray();

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isArray());
    ASSERT_TRUE(schema.getArrayItemSchema().isString());
    ASSERT_FALSE(schema.getArrayItemSchema().isUntyped());
}

TEST(ValueSchema, supportsMap) {
    auto schema = ValueSchema::map(ValueSchema::string(), ValueSchema::integer());

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_FALSE(schema.isString());
    ASSERT_TRUE(schema.isMap());
    ASSERT_EQ(ValueSchema::string(), schema.getMap()->getKey());
    ASSERT_EQ(ValueSchema::integer(), schema.getMap()->getValue());

    ASSERT_EQ(ValueSchema::map(ValueSchema::string(), ValueSchema::integer()), schema);

    ASSERT_NE(ValueSchema::map(ValueSchema::string().asOptional(), ValueSchema::integer()), schema);
    ASSERT_NE(ValueSchema::map(ValueSchema::string(), ValueSchema::integer().asOptional()), schema);
}

TEST(ValueSchema, supportsValueFunction) {
    auto schema = ValueSchema::function(ValueSchema::string().asOptional(),
                                        {ValueSchema::boolean(), ValueSchema::longInteger().asArray()});

    ASSERT_TRUE(schema.isFunction());

    const auto* functionSchema = schema.getFunction();
    ASSERT_TRUE(functionSchema != nullptr);

    ASSERT_TRUE(functionSchema->getReturnValue().isString());
    ASSERT_TRUE(functionSchema->getReturnValue().isOptional());

    ASSERT_EQ(static_cast<size_t>(2), functionSchema->getParametersSize());

    ASSERT_TRUE(functionSchema->getParameter(0).isBoolean());
    ASSERT_TRUE(functionSchema->getParameter(1).isArray());
    ASSERT_TRUE(functionSchema->getParameter(1).getArrayItemSchema().isLongInteger());
}

TEST(ValueSchema, supportsClass) {
    auto schema = ValueSchema::cls(
        STRING_LITERAL("MyClass"),
        false,
        {
            ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
            ClassPropertySchema(STRING_LITERAL("world"),
                                ValueSchema::function(ValueSchema::boolean(), {ValueSchema::doublePrecision()})),
        });

    ASSERT_TRUE(schema.isClass());

    const auto* classSchema = schema.getClass();
    ASSERT_TRUE(classSchema != nullptr);

    ASSERT_EQ(STRING_LITERAL("MyClass"), classSchema->getClassName());
    ASSERT_FALSE(classSchema->isInterface());
    ASSERT_EQ(static_cast<size_t>(2), classSchema->getPropertiesSize());

    ASSERT_EQ(STRING_LITERAL("hello"), classSchema->getProperty(0).name);
    ASSERT_TRUE(classSchema->getProperty(0).schema.isString());

    ASSERT_EQ(STRING_LITERAL("world"), classSchema->getProperty(1).name);
    ASSERT_TRUE(classSchema->getProperty(1).schema.isFunction());
    ASSERT_TRUE(classSchema->getProperty(1).schema.getFunction()->getReturnValue().isBoolean());
    ASSERT_EQ(static_cast<size_t>(1), classSchema->getProperty(1).schema.getFunction()->getParametersSize());
    ASSERT_TRUE(classSchema->getProperty(1).schema.getFunction()->getParameter(0).isDouble());

    schema = ValueSchema::cls(STRING_LITERAL("MyClass"),
                              true,
                              {
                                  ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                              });

    ASSERT_TRUE(schema.isClass());

    classSchema = schema.getClass();
    ASSERT_TRUE(classSchema != nullptr);

    ASSERT_TRUE(classSchema->isInterface());
    ASSERT_EQ(static_cast<size_t>(1), classSchema->getPropertiesSize());
}

TEST(ValueSchema, supportsIntEnum) {
    auto schema =
        ValueSchema::enumeration(STRING_LITERAL("StatusCode"),
                                 ValueSchema::integer(),
                                 {EnumCaseSchema(STRING_LITERAL("ok"), Value(static_cast<int32_t>(200))),
                                  EnumCaseSchema(STRING_LITERAL("not_found"), Value(static_cast<int32_t>(404))),
                                  EnumCaseSchema(STRING_LITERAL("internal_error"), Value(static_cast<int32_t>(500)))});

    ASSERT_TRUE(schema.isEnum());

    const auto* enumSchema = schema.getEnum();
    ASSERT_TRUE(enumSchema != nullptr);

    ASSERT_EQ(STRING_LITERAL("StatusCode"), enumSchema->getName());
    ASSERT_EQ(ValueSchema::integer(), enumSchema->getCaseSchema());
    ASSERT_EQ(static_cast<size_t>(3), enumSchema->getCasesSize());

    ASSERT_EQ(STRING_LITERAL("ok"), enumSchema->getCase(0).name);
    ASSERT_EQ(Value(static_cast<int32_t>(200)), enumSchema->getCase(0).value);

    ASSERT_EQ(STRING_LITERAL("not_found"), enumSchema->getCase(1).name);
    ASSERT_EQ(Value(static_cast<int32_t>(404)), enumSchema->getCase(1).value);

    ASSERT_EQ(STRING_LITERAL("internal_error"), enumSchema->getCase(2).name);
    ASSERT_EQ(Value(static_cast<int32_t>(500)), enumSchema->getCase(2).value);
}

TEST(ValueSchema, supportsStringEnum) {
    auto schema = ValueSchema::enumeration(
        STRING_LITERAL("Status"),
        ValueSchema::string(),
        {EnumCaseSchema(STRING_LITERAL("OK"), Value(STRING_LITERAL("All Good"))),
         EnumCaseSchema(STRING_LITERAL("NOT_OK"), Value(STRING_LITERAL("This is not great")))});

    ASSERT_TRUE(schema.isEnum());

    const auto* enumSchema = schema.getEnum();
    ASSERT_TRUE(enumSchema != nullptr);

    ASSERT_EQ(STRING_LITERAL("Status"), enumSchema->getName());
    ASSERT_EQ(ValueSchema::string(), enumSchema->getCaseSchema());
    ASSERT_EQ(static_cast<size_t>(2), enumSchema->getCasesSize());

    ASSERT_EQ(STRING_LITERAL("OK"), enumSchema->getCase(0).name);
    ASSERT_EQ(Value(STRING_LITERAL("All Good")), enumSchema->getCase(0).value);

    ASSERT_EQ(STRING_LITERAL("NOT_OK"), enumSchema->getCase(1).name);
    ASSERT_EQ(Value(STRING_LITERAL("This is not great")), enumSchema->getCase(1).value);
}

TEST(ValueSchema, supportsToString) {
    auto schema = ValueSchema::function(ValueSchema::string().asOptional(),
                                        {ValueSchema::boolean(), ValueSchema::longInteger().asArray()});

    ASSERT_EQ("func(bool, array<long>): string?", schema.toString());

    schema = ValueSchema::cls(
        STRING_LITERAL("MyClass"),
        false,
        {
            ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
            ClassPropertySchema(STRING_LITERAL("world"),
                                ValueSchema::function(ValueSchema::boolean(), {ValueSchema::doublePrecision()})),
        });

    ASSERT_EQ("class 'MyClass'{'hello': string, 'world': func(double): bool}", schema.toString());

    schema = ValueSchema::enumeration(STRING_LITERAL("Status"),
                                      ValueSchema::untyped(),
                                      {
                                          EnumCaseSchema(STRING_LITERAL("OK"), Value(STRING_LITERAL("All Good"))),
                                          EnumCaseSchema(STRING_LITERAL("NOT_FOUND"), Value(static_cast<int32_t>(404))),
                                      });

    ASSERT_EQ("enum<untyped> 'Status'{'OK': 'All Good', 'NOT_FOUND': 404}", schema.toString());

    schema = ValueSchema::map(ValueSchema::string().asOptional(), ValueSchema::boolean());

    ASSERT_EQ("map<string?, bool>", schema.toString());

    schema = ValueSchema::typeReference(
        ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHintEnum, STRING_LITERAL("MyEnum")));

    ASSERT_EQ("ref<enum>:'MyEnum'", schema.toString());
}

TEST(ValueSchema, supportsTypeReference) {
    auto schema = ValueSchema::typeReference(ValueSchemaTypeReference::positional(1));

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isTypeReference());

    ASSERT_TRUE(schema.getTypeReference().isPositional());
    ASSERT_EQ(1, schema.getTypeReference().getPosition());

    schema = ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Hello")));

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isTypeReference());

    ASSERT_FALSE(schema.getTypeReference().isPositional());
    ASSERT_EQ(STRING_LITERAL("Hello"), schema.getTypeReference().getName());
}

TEST(ValueSchema, supportsTypeReferenceWithTypeHint) {
    auto schema = ValueSchema::typeReference(
        ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHintEnum, STRING_LITERAL("MyEnum")));

    ASSERT_FALSE(schema.isUntyped());
    ASSERT_TRUE(schema.isTypeReference());
    ASSERT_FALSE(schema.getTypeReference().isPositional());
    ASSERT_EQ(STRING_LITERAL("MyEnum"), schema.getTypeReference().getName());
    ASSERT_EQ(ValueSchemaTypeReferenceTypeHintEnum, schema.getTypeReference().getTypeHint());

    ASSERT_EQ(schema,
              ValueSchema::typeReference(
                  ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHintEnum, STRING_LITERAL("MyEnum"))));
    ASSERT_NE(schema,
              ValueSchema::typeReference(
                  ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHintObject, STRING_LITERAL("MyEnum"))));
    ASSERT_NE(schema,
              ValueSchema::typeReference(
                  ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHintUnknown, STRING_LITERAL("MyEnum"))));
    ASSERT_NE(schema, ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("MyEnum"))));
}

TEST(ValueSchema, supportsEquals) {
    auto schema = ValueSchema::function(ValueSchema::string().asOptional(),
                                        {ValueSchema::boolean(), ValueSchema::longInteger().asArray()});

    ASSERT_NE(schema, ValueSchema::integer());
    ASSERT_NE(schema, ValueSchema::longInteger().asArray());
    ASSERT_NE(schema, ValueSchema::longInteger().asOptional());
    ASSERT_EQ(schema, schema);
    ASSERT_EQ(schema,
              ValueSchema::function(ValueSchema::string().asOptional(),
                                    {ValueSchema::boolean(), ValueSchema::longInteger().asArray()}));

    auto valueMapSchema = ValueSchema::cls(STRING_LITERAL("MyClass"),
                                           false,
                                           {
                                               ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                                           });

    ASSERT_NE(valueMapSchema, ValueSchema::integer());
    ASSERT_NE(valueMapSchema, ValueSchema::longInteger().asArray());
    ASSERT_NE(valueMapSchema, ValueSchema::longInteger().asOptional());
    ASSERT_EQ(valueMapSchema, valueMapSchema);
    ASSERT_EQ(valueMapSchema,
              ValueSchema::cls(STRING_LITERAL("MyClass"),
                               false,
                               {
                                   ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                               }));
    ASSERT_NE(valueMapSchema,
              ValueSchema::cls(STRING_LITERAL("MyClass"),
                               false,
                               {
                                   ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::boolean()),
                               }));

    ASSERT_NE(valueMapSchema,
              ValueSchema::cls(STRING_LITERAL("MyClass"),
                               false,
                               {
                                   ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                                   ClassPropertySchema(STRING_LITERAL("world"), ValueSchema::string()),
                               }));
}

TEST(ValueSchema, resolvesTypeReference) {
    auto registry = makeShared<ValueSchemaRegistry>();

    auto valueMapSchema = ValueSchema::cls(STRING_LITERAL("MyClass"),
                                           false,
                                           {
                                               ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                                           });

    registry->registerSchema(STRING_LITERAL("MyClass"), valueMapSchema);

    auto schema = ValueSchema::typeReference(ValueSchemaTypeReference::positional(0));

    ASSERT_TRUE(schema.isTypeReference());

    ValueSchemaTypeResolver typeResolver(registry.get());
    auto result =
        typeResolver.resolveTypeReferences(schema, ValueSchemaRegistryResolveType::Schema, {ValueSchema::integer()});

    ASSERT_TRUE(result.success()) << result.description();
    schema = result.moveValue();
    ASSERT_FALSE(schema.isTypeReference());
    ASSERT_TRUE(schema.isInteger());
    ASSERT_FALSE(schema.isBoxed());

    schema = ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("MyClass")));

    ASSERT_TRUE(schema.isTypeReference());

    result = typeResolver.resolveTypeReferences(schema);
    ASSERT_TRUE(result.success()) << result.description();
    schema = result.moveValue();
    ASSERT_FALSE(schema.isTypeReference());
    ASSERT_TRUE(schema.isSchemaReference());
    ASSERT_FALSE(schema.isBoxed());
    ASSERT_EQ(schema.getReferencedSchema(), valueMapSchema);
}

TEST(ValueSchema, resolvesOptionalTypeReference) {
    auto registry = makeShared<ValueSchemaRegistry>();

    auto valueMapSchema = ValueSchema::cls(STRING_LITERAL("MyClass"),
                                           false,
                                           {
                                               ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                                           });

    registry->registerSchema(STRING_LITERAL("MyClass"), valueMapSchema);

    auto schema = ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("MyClass"))).asOptional();

    ASSERT_TRUE(schema.isTypeReference());

    ValueSchemaTypeResolver typeResolver(registry.get());
    auto result = typeResolver.resolveTypeReferences(schema);
    ASSERT_TRUE(result.success()) << result.description();
    schema = result.moveValue();
    ASSERT_FALSE(schema.isTypeReference());
    ASSERT_TRUE(schema.isOptional());
    ASSERT_TRUE(schema.isSchemaReference());
    ASSERT_FALSE(schema.isBoxed());
    ASSERT_EQ(schema.getReferencedSchema(), valueMapSchema);
}

TEST(ValueSchema, failsResolveTypeReferenceWhenTypeIsNotPresent) {
    auto registry = makeShared<ValueSchemaRegistry>();

    auto valueMapSchema = ValueSchema::cls(STRING_LITERAL("MyClass"),
                                           false,
                                           {
                                               ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                                           });

    registry->registerSchema(STRING_LITERAL("MyClass"), valueMapSchema);

    auto schema = ValueSchema::typeReference(ValueSchemaTypeReference::positional(1));

    ASSERT_TRUE(schema.isTypeReference());

    ValueSchemaTypeResolver typeResolver(registry.get());
    auto result =
        typeResolver.resolveTypeReferences(schema, ValueSchemaRegistryResolveType::Schema, {ValueSchema::integer()});

    ASSERT_FALSE(result.success()) << result.description();
}

TEST(ValueSchema, canResolveTypeReferenceOfUnregisteredTypesThroughListenerCallback) {
    auto registry = makeShared<ValueSchemaRegistry>();
    auto listener = makeShared<TestValueSchemaRegistryListener>();
    registry->setListener(listener);

    auto classSchema = ValueSchema::parse("c 'MyClass' {'type': r:'MyEnum'}");
    ASSERT_TRUE(classSchema) << classSchema.description();

    auto enumSchema = ValueSchema::parse("e<s> 'MyEnum' {'good': 'OK', 'bad': 'NOT OK'}");
    ASSERT_TRUE(enumSchema) << enumSchema.description();

    ValueSchemaTypeResolver typeResolver(registry.get());
    auto result = typeResolver.resolveTypeReferences(classSchema.value());

    // Should initially fail as MyEnum is not registered
    ASSERT_FALSE(result) << result.description();

    size_t callbackCallCount = 0;

    // Set a callback in the listener that should be called to ask us to register the enum
    listener->setCallback(
        ValueSchemaRegistryKey(ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("MyEnum")))),
        [&]() -> Void {
            callbackCallCount++;
            registry->registerSchema(enumSchema.value());
            return Void();
        });

    // Now try again
    result = typeResolver.resolveTypeReferences(classSchema.value());

    // Should now succeed
    ASSERT_TRUE(result) << result.description();

    ASSERT_EQ(static_cast<size_t>(1), callbackCallCount);

    ASSERT_EQ("class 'MyClass'{'type': link:ref:'MyEnum'}", result.value().toString());
    ASSERT_EQ("class 'MyClass'{'type': link:enum<string> 'MyEnum'{'good': 'OK', 'bad': 'NOT OK'}}",
              result.value().toString(ValueSchemaPrintFormat::HumanReadable, true));
}

TEST(ValueSchema, resolvesNestedTypeReferences) {
    auto registry = makeShared<ValueSchemaRegistry>();

    auto valueMapSchema = ValueSchema::cls(STRING_LITERAL("MyClass"),
                                           false,
                                           {
                                               ClassPropertySchema(STRING_LITERAL("hello"), ValueSchema::string()),
                                           });

    ValueSchemaTypeResolver typeResolver(registry.get());
    registry->registerSchema(STRING_LITERAL("MyClass"), valueMapSchema);

    auto nestedSchema = ValueSchema::cls(
        STRING_LITERAL("Nested"),
        false,
        {ClassPropertySchema(STRING_LITERAL("otherArray"),
                             ValueSchema::array(ValueSchema::typeReference(ValueSchemaTypeReference::positional(1))))});

    auto schema = ValueSchema::cls(
        STRING_LITERAL("OtherClass"),
        false,
        {
            ClassPropertySchema(
                STRING_LITERAL("myFunction"),
                ValueSchema::function(ValueSchema::typeReference(ValueSchemaTypeReference::positional(0)),
                                      {
                                          ValueSchema::typeReference(ValueSchemaTypeReference::positional(1)),
                                      })),

            ClassPropertySchema(STRING_LITERAL("myArray"),
                                ValueSchema::array(ValueSchema::typeReference(
                                    ValueSchemaTypeReference::named(STRING_LITERAL("MyClass"))))),

            ClassPropertySchema(STRING_LITERAL("myArrayOfNested"), ValueSchema::array(nestedSchema)),
        });

    ASSERT_EQ("class 'OtherClass'{'myFunction': func(ref:1): ref:0, 'myArray': array<ref:'MyClass'>, "
              "'myArrayOfNested': array<class 'Nested'{'otherArray': array<ref:1>}>}",
              schema.toString());

    ASSERT_EQ("class 'OtherClass'{'myFunction': func(ref:1): ref:0, 'myArray': array<ref:'MyClass'>, "
              "'myArrayOfNested': array<class 'Nested'{'otherArray': array<ref:1>}>}",
              schema.toString(ValueSchemaPrintFormat::HumanReadable, true));

    auto result = typeResolver.resolveTypeReferences(
        schema, ValueSchemaRegistryResolveType::Schema, {ValueSchema::integer(), ValueSchema::boolean()});
    ASSERT_TRUE(result.success()) << result.description();
    auto resolvedSchema = result.moveValue();

    ASSERT_EQ("class 'OtherClass'{'myFunction': func(bool): int, 'myArray': array<link:ref:'MyClass'>, "
              "'myArrayOfNested': array<class 'Nested'{'otherArray': array<bool>}>}",
              resolvedSchema.toString());

    ASSERT_EQ("class 'OtherClass'{'myFunction': func(bool): int, 'myArray': array<link:class 'MyClass'{'hello': "
              "string}>, 'myArrayOfNested': array<class 'Nested'{'otherArray': array<bool>}>}",
              resolvedSchema.toString(ValueSchemaPrintFormat::HumanReadable, true));

    // Boxing should be preserved
    result = typeResolver.resolveTypeReferences(schema,
                                                ValueSchemaRegistryResolveType::Schema,
                                                {ValueSchema::integer().asBoxed(), ValueSchema::boolean().asBoxed()});
    ASSERT_TRUE(result.success()) << result.description();
    resolvedSchema = result.moveValue();

    ASSERT_EQ("class 'OtherClass'{'myFunction': func(bool@): int@, 'myArray': array<link:ref:'MyClass'>, "
              "'myArrayOfNested': array<class 'Nested'{'otherArray': array<bool@>}>}",
              resolvedSchema.toString());

    ASSERT_EQ("class 'OtherClass'{'myFunction': func(bool@): int@, 'myArray': array<link:class 'MyClass'{'hello': "
              "string}>, 'myArrayOfNested': array<class 'Nested'{'otherArray': array<bool@>}>}",
              resolvedSchema.toString(ValueSchemaPrintFormat::HumanReadable, true));
}

TEST(ValueSchema, resolvesGenericTypeReferenceWithResolvedValue) {
    auto registry = makeShared<ValueSchemaRegistry>();
    ValueSchemaTypeResolver typeResolver(registry.get());

    auto observableSchema =
        ValueSchema::cls(STRING_LITERAL("Observable"),
                         false,
                         {
                             ClassPropertySchema(STRING_LITERAL("value"),
                                                 ValueSchema::typeReference(ValueSchemaTypeReference::positional(0))),
                         });

    registry->registerSchema(STRING_LITERAL("Observable"), observableSchema);

    auto schema =
        ValueSchema::cls(STRING_LITERAL("Store"),
                         false,
                         {
                             ClassPropertySchema(STRING_LITERAL("observable"),
                                                 ValueSchema::genericTypeReference(
                                                     ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                                                     {ValueSchema::integer().asBoxed()})),
                         });

    auto result = typeResolver.resolveTypeReferences(schema);
    ASSERT_TRUE(result.success()) << result.description();
    schema = result.moveValue();

    ASSERT_EQ("class 'Store'{'observable': link:genref:'Observable'<int@>}", schema.toString());

    ASSERT_EQ("class 'Store'{'observable': link:class 'Observable'{'value': int@}}",
              schema.toString(ValueSchemaPrintFormat::HumanReadable, true));
}

TEST(ValueSchema, resolvesComplexGenericTypeReferences) {
    auto registry = makeShared<ValueSchemaRegistry>();
    ValueSchemaTypeResolver typeResolver(registry.get());

    auto observableSchema =
        ValueSchema::cls(STRING_LITERAL("Observable"),
                         false,
                         {
                             ClassPropertySchema(STRING_LITERAL("value"),
                                                 ValueSchema::typeReference(ValueSchemaTypeReference::positional(0))),
                             ClassPropertySchema(STRING_LITERAL("value2"),
                                                 ValueSchema::typeReference(ValueSchemaTypeReference::positional(1))),
                         });

    auto storeSchema = ValueSchema::cls(
        STRING_LITERAL("Store"),
        false,
        {ClassPropertySchema(
             STRING_LITERAL("observable"),
             ValueSchema::genericTypeReference(
                 ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                 {
                     ValueSchema::typeReference(ValueSchemaTypeReference::positional(2)),
                     ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))),
                 })),
         ClassPropertySchema(STRING_LITERAL("observableOfObservables"),
                             ValueSchema::genericTypeReference(
                                 ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                                 {
                                     ValueSchema::array(ValueSchema::genericTypeReference(
                                         ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                                         {
                                             ValueSchema::typeReference(ValueSchemaTypeReference::positional(1)),
                                             ValueSchema::typeReference(ValueSchemaTypeReference::positional(0)),
                                         })),
                                     ValueSchema::typeReference(ValueSchemaTypeReference::positional(2)),
                                 }))});

    auto friendSchema = ValueSchema::cls(
        STRING_LITERAL("Friend"),
        false,
        {
            ClassPropertySchema(STRING_LITERAL("isConnected"),
                                ValueSchema::function(ValueSchema::genericTypeReference(
                                                          ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                                                          {ValueSchema::string(), ValueSchema::integer().asBoxed()}),
                                                      {})),
        });

    auto friendStoreSchema = ValueSchema::cls(
        STRING_LITERAL("FriendStore"),
        false,
        {ClassPropertySchema(STRING_LITERAL("store"),
                             ValueSchema::genericTypeReference(
                                 ValueSchemaTypeReference::named(STRING_LITERAL("Store")),
                                 {ValueSchema::doublePrecision().asBoxed(),
                                  ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))),
                                  ValueSchema::boolean()}))

        });

    registry->registerSchema(observableSchema);
    registry->registerSchema(storeSchema);
    registry->registerSchema(friendSchema);
    auto friendStoreSchemaIdentifier = registry->registerSchema(friendStoreSchema);

    ASSERT_EQ("class 'Observable'{'value': ref:0, 'value2': ref:1}", observableSchema.toString());
    ASSERT_EQ("class 'Store'{'observable': genref:'Observable'<ref:2, ref:'Friend'>, 'observableOfObservables': "
              "genref:'Observable'<array<genref:'Observable'<ref:1, ref:0>>, ref:2>}",
              storeSchema.toString());
    ASSERT_EQ("class 'Friend'{'isConnected': func(): genref:'Observable'<string, int@>}", friendSchema.toString());
    ASSERT_EQ("class 'FriendStore'{'store': genref:'Store'<double@, ref:'Friend', bool>}",
              friendStoreSchema.toString());

    auto result = typeResolver.resolveTypeReferences(friendStoreSchema);

    ASSERT_TRUE(result) << result.description();
    friendStoreSchema = result.moveValue();

    ASSERT_EQ("class 'FriendStore'{'store': link:genref:'Store'<double@, ref:'Friend', bool>}",
              friendStoreSchema.toString());
    ASSERT_EQ("class 'FriendStore'{'store': link:class 'Store'{'observable': link:class 'Observable'{'value': bool, "
              "'value2': link:class 'Friend'{'isConnected': func(): genref:'Observable'<string, int@>}}, "
              "'observableOfObservables': link:class 'Observable'{'value': array<link:class 'Observable'{'value': "
              "link:ref:'Friend', 'value2': double@}>, 'value2': bool}}}",
              friendStoreSchema.toString(ValueSchemaPrintFormat::HumanReadable, true));

    // The other schemas should not have changed
    ASSERT_EQ("class 'Observable'{'value': ref:0, 'value2': ref:1}", observableSchema.toString());
    ASSERT_EQ("class 'Store'{'observable': genref:'Observable'<ref:2, ref:'Friend'>, 'observableOfObservables': "
              "genref:'Observable'<array<genref:'Observable'<ref:1, ref:0>>, ref:2>}",
              storeSchema.toString());
    ASSERT_EQ("class 'Friend'{'isConnected': func(): genref:'Observable'<string, int@>}", friendSchema.toString());

    auto allSchemas = registry->getAllSchemas();

    ASSERT_EQ(static_cast<size_t>(8), allSchemas.size());

    // friend store should not be resolved as we didn't resolve the type reference inside the registry directly
    ASSERT_EQ("class 'FriendStore'{'store': genref:'Store'<double@, ref:'Friend', bool>}", allSchemas[3].toString());

    // Now resolve the friend store that lives inside the registry
    registry->updateSchema(friendStoreSchemaIdentifier, friendStoreSchema);

    allSchemas = registry->getAllSchemas();

    // Number of schemas should not have changed
    ASSERT_EQ(static_cast<size_t>(8), allSchemas.size());

    ASSERT_EQ("class 'Observable'{'value': ref:0, 'value2': ref:1}", allSchemas[0].toString());
    ASSERT_EQ("class 'Store'{'observable': genref:'Observable'<ref:2, ref:'Friend'>, 'observableOfObservables': "
              "genref:'Observable'<array<genref:'Observable'<ref:1, ref:0>>, ref:2>}",
              allSchemas[1].toString());
    ASSERT_EQ("class 'Friend'{'isConnected': func(): genref:'Observable'<string, int@>}", allSchemas[2].toString());
    ASSERT_EQ("class 'FriendStore'{'store': link:genref:'Store'<double@, ref:'Friend', bool>}",
              allSchemas[3].toString());

    // We should have dedicated instantiation of each generics
    ASSERT_EQ("class 'Store'{'observable': link:genref:'Observable'<bool, ref:'Friend'>, 'observableOfObservables': "
              "link:genref:'Observable'<array<genref:'Observable'<ref:'Friend', double@>>, bool>}",
              allSchemas[4].toString());
    ASSERT_EQ("class 'Observable'{'value': bool, 'value2': link:ref:'Friend'}", allSchemas[5].toString());
    ASSERT_EQ("class 'Observable'{'value': array<link:genref:'Observable'<ref:'Friend', double@>>, 'value2': bool}",
              allSchemas[6].toString());
    ASSERT_EQ("class 'Observable'{'value': link:ref:'Friend', 'value2': double@}", allSchemas[7].toString());

    auto schemaKeys = registry->getAllSchemaKeys();

    ASSERT_EQ(static_cast<size_t>(8), schemaKeys.size());

    ASSERT_EQ(ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Observable"))), schemaKeys[0]);
    ASSERT_EQ(ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Store"))), schemaKeys[1]);
    ASSERT_EQ(ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))), schemaKeys[2]);
    ASSERT_EQ(ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("FriendStore"))),
              schemaKeys[3]);
    ASSERT_EQ(ValueSchema::genericTypeReference(
                  ValueSchemaTypeReference::named(STRING_LITERAL("Store")),
                  {
                      ValueSchema::doublePrecision().asBoxed(),
                      ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))),
                      ValueSchema::boolean(),
                  }),
              schemaKeys[4]);

    ASSERT_EQ(ValueSchema::genericTypeReference(
                  ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                  {
                      ValueSchema::boolean(),
                      ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))),
                  }),
              schemaKeys[5]);

    ASSERT_EQ(ValueSchema::genericTypeReference(
                  ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                  {
                      ValueSchema::array(ValueSchema::genericTypeReference(
                          ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                          {
                              ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))),
                              ValueSchema::doublePrecision().asBoxed(),
                          })),
                      ValueSchema::boolean(),
                  }),
              schemaKeys[6]);

    ASSERT_EQ(ValueSchema::genericTypeReference(
                  ValueSchemaTypeReference::named(STRING_LITERAL("Observable")),
                  {
                      ValueSchema::typeReference(ValueSchemaTypeReference::named(STRING_LITERAL("Friend"))),
                      ValueSchema::doublePrecision().asBoxed(),
                  }),
              schemaKeys[7]);
}

TEST(ValueSchema, canParseUntypedSchema) {
    auto result = ValueSchema::parse("u");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("untyped", result.value().toString());
}

TEST(ValueSchema, canParseVoidSchema) {
    auto result = ValueSchema::parse("v");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("void", result.value().toString());
}

TEST(ValueSchema, canParseIntSchema) {
    auto result = ValueSchema::parse("i");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("int", result.value().toString());
}

TEST(ValueSchema, canParseLongSchema) {
    auto result = ValueSchema::parse("l");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("long", result.value().toString());
}

TEST(ValueSchema, canParseDoubleSchema) {
    auto result = ValueSchema::parse("d");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("double", result.value().toString());
}

TEST(ValueSchema, canParseBooleanSchema) {
    auto result = ValueSchema::parse("b");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("bool", result.value().toString());
}

TEST(ValueSchema, canParseStringSchema) {
    auto result = ValueSchema::parse("s");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("string", result.value().toString());
}

TEST(ValueSchema, canParseValueTypedArraySchema) {
    auto result = ValueSchema::parse("t");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("typedarray", result.value().toString());
}

TEST(ValueSchema, canParsePositionalTypeReferenceSchema) {
    auto result = ValueSchema::parse("r:1");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("ref:1", result.value().toString());
}

TEST(ValueSchema, canParseNamedTypeReferenceSchema) {
    auto result = ValueSchema::parse("r:'Hello'");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("ref:'Hello'", result.value().toString());

    result = ValueSchema::parse("r<e>:'MyEnum'");
    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("ref<enum>:'MyEnum'", result.value().toString());

    result = ValueSchema::parse("r<o>:'MyClass'");
    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("ref<object>:'MyClass'", result.value().toString());

    result = ValueSchema::parse("r<u>:'MyClass'");
    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("ref:'MyClass'", result.value().toString());
}

TEST(ValueSchema, canParseGenericTypeReferenceSchema) {
    auto result = ValueSchema::parse("g:'Observable'<i>");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("genref:'Observable'<int>", result.value().toString());

    result = ValueSchema::parse("g:'SomeType'<r:1, l>");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("genref:'SomeType'<ref:1, long>", result.value().toString());

    result = ValueSchema::parse("g:'SomeType'<>");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("genref:'SomeType'<>", result.value().toString());
}

TEST(ValueSchema, canParseClassSchema) {
    auto result = ValueSchema::parse("c 'Observable'{'value':r:1}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("class 'Observable'{'value': ref:1}", result.value().toString());

    result = ValueSchema::parse("c 'Observable'{'value': r:1, 'value2': t}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("class 'Observable'{'value': ref:1, 'value2': typedarray}", result.value().toString());
}

TEST(ValueSchema, canParseProtocolSchema) {
    auto result = ValueSchema::parse("c+ 'Listener'{'shouldContinue': f(): b}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("class+ 'Listener'{'shouldContinue': func(): bool}", result.value().toString());
}

TEST(ValueSchema, canParseEnumSchema) {
    auto result = ValueSchema::parse("e<i> 'StatusCode'{'ok':200, 'not_found': 404}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("enum<int> 'StatusCode'{'ok': 200, 'not_found': 404}", result.value().toString());

    result = ValueSchema::parse("e<s> 'StatusCode'{'ok':'OKI', 'not_found': 'NOT OKI'}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("enum<string> 'StatusCode'{'ok': 'OKI', 'not_found': 'NOT OKI'}", result.value().toString());

    result = ValueSchema::parse("e<u> 'SomeWeirdEmptyEnum'{}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("enum<untyped> 'SomeWeirdEmptyEnum'{}", result.value().toString());
}

TEST(ValueSchema, canParseFunctionSchema) {
    auto result = ValueSchema::parse("f()");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("func(): void", result.value().toString());

    result = ValueSchema::parse("f(i): l");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("func(int): long", result.value().toString());

    result = ValueSchema::parse("f(i, b):f(t):u");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("func(int, bool): func(typedarray): untyped", result.value().toString());

    result = ValueSchema::parse("f|m|(i)");
    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("func|method|(int): void", result.value().toString());

    result = ValueSchema::parse("f|s|(i)");
    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("func|singlecall|(int): void", result.value().toString());
}

TEST(ValueSchema, canParseArraySchema) {
    auto result = ValueSchema::parse("a<l>");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("array<long>", result.value().toString());
}

TEST(ValueSchema, canParseMapSchema) {
    auto result = ValueSchema::parse("m<s?, b@>");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("map<string?, bool@>", result.value().toString());
}

TEST(ValueSchema, canParseOptional) {
    auto result = ValueSchema::parse("l?");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("long?", result.value().toString());

    result = ValueSchema::parse("a?<l>");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("array?<long>", result.value().toString());

    result = ValueSchema::parse("f?(l, i?)");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("func?(long, int?): void", result.value().toString());
}

TEST(ValueSchema, canParseComplexSchema) {
    auto result = ValueSchema::parse("c 'Observable'{'value':f?(f?(g:0<i>)), 'inlineSchema': c? "
                                     "'AnotherSchema'{'getSomething': f(): a<a<g:'Observable'<g:0<l>, i>>>}}");

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ("class 'Observable'{'value': func?(func?(genref:0<int>): void): void, 'inlineSchema': class? "
              "'AnotherSchema'{'getSomething': func(): array<array<genref:'Observable'<genref:0<long>, int>>>}}",
              result.value().toString());
}

TEST(ValueSchema, failsWhenParsingSchemaReference) {
    auto result = ValueSchema::parse("l:r:'Hello'");
    ASSERT_FALSE(result) << result.description();
}

TEST(ValueSchema, failsOnMalformatedArgumentLists) {
    auto result = ValueSchema::parse("c 'Observable'{'value': r:1, }");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("a<>");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("f(,)");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("f(l,)");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("f(l i)");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("g:'SomeType'<r:1 l>");

    ASSERT_FALSE(result);
}

TEST(ValueSchema, failsOnUexpectedTrailingCharacters) {
    auto result = ValueSchema::parse("a<l>  ");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("a<l>i");

    ASSERT_FALSE(result);
}

TEST(ValueSchema, failsOnMalformatedMap) {
    auto result = ValueSchema::parse("m");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("m<>");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("m<s>");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("m<s, s, s>");

    ASSERT_FALSE(result);
}

TEST(ValueSchema, failsOnInvalidTokens) {
    auto result = ValueSchema::parse("hello");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("world");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("f(");

    ASSERT_FALSE(result);

    result = ValueSchema::parse("f");

    ASSERT_FALSE(result);
}

} // namespace ValdiTest
