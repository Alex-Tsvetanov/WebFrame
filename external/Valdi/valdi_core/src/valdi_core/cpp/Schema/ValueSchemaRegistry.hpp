//
//  ValueSchemaRegistry.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/17/23.
//

#pragma once

#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Schema/ValueSchemaRegistryKey.hpp"
#include "valdi_core/cpp/Schema/ValueSchemaRegistrySchemaIdentifier.hpp"
#include "valdi_core/cpp/Schema/ValueSchemaTypeReference.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include <vector>

namespace Valdi {

class ValueSchemaRegistry;

class ValueSchemaRegistryReference;

struct ValueSchemaRegistryEntry {
    ValueSchema schema;
    Ref<ValueSchemaRegistryReference> reference;
};

struct RegisteredValueSchema {
    ValueSchemaRegistryKey schemaKey;
    ValueSchema schema;

    inline RegisteredValueSchema(const ValueSchemaRegistryKey& schemaKey, const ValueSchema& schema)
        : schemaKey(schemaKey), schema(schema) {}
};

class ValueSchemaRegistry;

class ValueSchemaRegistryListener : public SimpleRefCountable {
public:
    /**
     Will be called by the ValueSchemaRegistry when a schema is requested and doesn't exist the registry.
     The listener can implement this method to register the schema inside the registry and return whether
     the registration was succesful. This is helpful to implement lazy registration of schemas.
     */
    virtual Result<Void> resolveSchemaIdentifierForSchemaKey(ValueSchemaRegistry& registry,
                                                             const ValueSchemaRegistryKey& registryKey) = 0;
};

/**
 The ValueSchemaRegistry holds registered schemas and resolve
 the references between them. Each schema is registered with a key,
 which for a ValueMap schema will be a type ref to the type name of the schema,
 and for the generic type instance it will be a generic type reference with
 the type name of the schema and the resolved type arguments.
 */
class ValueSchemaRegistry : public SharedPtrRefCountable {
public:
    ValueSchemaRegistry();
    ~ValueSchemaRegistry() override;

    std::unique_lock<std::recursive_mutex> lock() const;

    std::optional<ValueSchema> getSchemaForTypeKey(const ValueSchemaRegistryKey& typeKey) const;
    std::optional<ValueSchema> getSchemaForTypeName(const StringBox& typeName) const;
    Ref<ValueSchemaReference> getSchemaReferenceForTypeKey(const ValueSchemaRegistryKey& typeKey) const;
    Ref<ValueSchemaReference> getSchemaReferenceForSchemaIdentifier(
        ValueSchemaRegistrySchemaIdentifier identifier) const;
    Result<Ref<ValueSchemaReference>> getOrResolveSchemaReferenceForTypeKey(const ValueSchemaRegistryKey& typeKey);

    std::vector<ValueSchema> getAllSchemas() const;
    std::vector<ValueSchema> getAllSchemaKeys() const;

    void setListener(const Ref<ValueSchemaRegistryListener>& listener);

    ValueSchemaRegistrySchemaIdentifier registerSchema(const StringBox& typeName, const ValueSchema& schema);
    ValueSchemaRegistrySchemaIdentifier registerSchema(const ValueSchema& schema);
    ValueSchemaRegistrySchemaIdentifier registerSchema(const ValueSchema& schemaKey, const ValueSchema& schema);
    ValueSchemaRegistrySchemaIdentifier registerSchema(const ValueSchemaRegistryKey& schemaKey,
                                                       const ValueSchema& schema);

    void unregisterSchema(ValueSchemaRegistrySchemaIdentifier identifier);

    void updateSchema(ValueSchemaRegistrySchemaIdentifier identifier, const ValueSchema& schema);
    bool updateSchemaIfKeyExists(const ValueSchemaRegistryKey& schemaKey, const ValueSchema& schema);

    std::optional<RegisteredValueSchema> getSchemaAndKeyForIdentifier(ValueSchemaRegistrySchemaIdentifier index) const;
    ValueSchema getSchemaForIdentifier(ValueSchemaRegistrySchemaIdentifier index) const;

    static Ref<ValueSchemaRegistry> sharedInstance();

private:
    mutable std::recursive_mutex _mutex;
    FlatMap<ValueSchemaRegistryKey, size_t> _entryIndexByKey;
    std::vector<ValueSchemaRegistryEntry> _entries;
    Ref<ValueSchemaRegistryListener> _listener;

    friend ValueSchemaRegistryReference;

    Ref<ValueSchemaReference> getSchemaPostRegistration(const ValueSchemaRegistryKey& typeKey) const;
};

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::ValueSchemaRegistryKey> {
    std::size_t operator()(const Valdi::ValueSchemaRegistryKey& k) const noexcept;
};

} // namespace std
