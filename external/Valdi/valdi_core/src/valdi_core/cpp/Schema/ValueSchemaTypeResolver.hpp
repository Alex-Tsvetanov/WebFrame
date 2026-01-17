//
//  ValueSchemaTypeResolver.hpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 2/2/23.
//

#pragma once

#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Schema/ValueSchemaRegistryKey.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {

class ValueSchemaRegistry;

enum class ValueSchemaRegistryResolveType {
    /**
     Resolves the type references such that the resulting
     schema is suitable for use in as a Key.
     */
    Key,

    /**
     Resolves the type references such that the resulting
     schema is suitable for use as the main schema for a type.
     */
    Schema,
};

struct ValueSchemaRegistryResolveOutput {
    ValueSchema schema;
    /**
     Whether the schema is different than the schema instance
     that was passed in.
     */
    bool didChange;

    inline ValueSchemaRegistryResolveOutput(ValueSchema&& schema, bool didChange)
        : schema(std::move(schema)), didChange(didChange) {}
    inline ValueSchemaRegistryResolveOutput(const ValueSchema& schema, bool didChange)
        : schema(schema), didChange(didChange) {}
};

/**
 The ValueSchemaTypeResolver can resolve type references from ValueSchema using an associated registry.
 If the registry is not set, the type resolver will fail when encountering a not completely resolved schema.
 */
class ValueSchemaTypeResolver {
public:
    ValueSchemaTypeResolver();
    explicit ValueSchemaTypeResolver(ValueSchemaRegistry* registry);

    Result<ValueSchema> resolveTypeReferences(
        const ValueSchema& schema,
        ValueSchemaRegistryResolveType resolveType = ValueSchemaRegistryResolveType::Schema,
        bool* didChange = nullptr) const;

    Result<ValueSchema> resolveTypeReferences(const ValueSchema& schema,
                                              ValueSchemaRegistryResolveType resolveType,
                                              const ValueSchema* typeArguments,
                                              size_t typeArgumentsLength,
                                              bool* didChange = nullptr) const;
    Result<ValueSchema> resolveTypeReferences(const ValueSchema& schema,
                                              ValueSchemaRegistryResolveType resolveType,
                                              std::initializer_list<ValueSchema> typeArguments) const {
        return resolveTypeReferences(schema, resolveType, typeArguments.begin(), typeArguments.size());
    }

    ValueSchemaRegistry* getRegistry() const;

private:
    ValueSchemaRegistry* _registry = nullptr;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferences(const ValueSchemaRegistryKey& currentKey,
                                                                   ValueSchemaRegistryResolveType resolveType,
                                                                   const ValueSchema& schema) const;
    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInner(const ValueSchemaRegistryKey& currentKey,
                                                                        ValueSchemaRegistryResolveType resolveType,
                                                                        const ValueSchema& schema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInFunction(const ValueSchemaRegistryKey& currentKey,
                                                                             ValueSchemaRegistryResolveType resolveType,
                                                                             const ValueSchema& sourceSchema,
                                                                             const ValueFunctionSchema& function) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInClass(const ValueSchemaRegistryKey& currentKey,
                                                                          ValueSchemaRegistryResolveType resolveType,
                                                                          const ValueSchema& sourceSchema,
                                                                          const ClassSchema& classSchema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveSchemaForGenericType(const ValueSchemaRegistryKey& currentKey,
                                                                         ValueSchemaRegistryResolveType resolveType,
                                                                         const GenericValueSchemaTypeReference& generic,
                                                                         bool asBoxed) const;

    Result<ValueSchemaRegistryResolveOutput> getResolvedSchemaForTypeReference(
        const ValueSchemaRegistryKey& currentKey,
        ValueSchemaRegistryResolveType resolveType,
        const ValueSchemaTypeReference& typeReference,
        bool asBoxed) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInArray(const ValueSchemaRegistryKey& currentKey,
                                                                          ValueSchemaRegistryResolveType resolveType,
                                                                          const ValueSchema& sourceSchema,
                                                                          const ValueSchema& itemSchema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInPromise(const ValueSchemaRegistryKey& currentKey,
                                                                            ValueSchemaRegistryResolveType resolveType,
                                                                            const ValueSchema& sourceSchema,
                                                                            const ValueSchema& valueSchema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInMap(const ValueSchemaRegistryKey& currentKey,
                                                                        ValueSchemaRegistryResolveType resolveType,
                                                                        const ValueSchema& sourceSchema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInES6Map(const ValueSchemaRegistryKey& currentKey,
                                                                           ValueSchemaRegistryResolveType resolveType,
                                                                           const ValueSchema& sourceSchema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInES6Set(const ValueSchemaRegistryKey& currentKey,
                                                                           ValueSchemaRegistryResolveType resolveType,
                                                                           const ValueSchema& sourceSchema) const;

    Result<ValueSchemaRegistryResolveOutput> resolveTypeReferencesInOutcome(const ValueSchemaRegistryKey& currentKey,
                                                                            ValueSchemaRegistryResolveType resolveType,
                                                                            const ValueSchema& sourceSchema,
                                                                            const ValueSchema& itemSchema,
                                                                            const ValueSchema& errorSchema) const;

    static ValueSchema getSchemaFromReference(const Ref<ValueSchemaReference>& schemaReference);
};

} // namespace Valdi
