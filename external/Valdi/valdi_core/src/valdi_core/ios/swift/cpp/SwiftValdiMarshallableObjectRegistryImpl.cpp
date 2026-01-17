//
//  SwiftValdiMarshallerRegistry.cpp
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//

#include "valdi_core/ios/swift/cpp/SwiftValdiMarshallableObjectRegistryImpl.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace ValdiSwift {

static Valdi::Result<Valdi::ValueSchema> schemaFromObjectDescriptor(
    const Valdi::StringBox className, const SwiftValdiMarshallableObjectRegistry_ObjectDescriptor& objectDescriptor) {
    Valdi::SmallVector<std::pair<std::string, std::string>, 8> replacementPairs;
    // Replace the identifiers inside the schema string.
    // This is used mostly to make the schema string description shorter
    // and use slightly less app binary size.
    const auto* const* identifiers = objectDescriptor.identifiers;
    if (identifiers != nullptr) {
        size_t identifierIndex = 0;
        while (*identifiers != nullptr) {
            replacementPairs.emplace_back(
                std::make_pair(fmt::format("[{}]", identifierIndex), std::string(std::string_view(*identifiers))));

            identifiers++;
            identifierIndex++;
        }
    }

    Valdi::SmallVector<Valdi::ClassPropertySchema, 16> properties;

    // Build the class schema string representation using the given fields
    const auto* fieldDescriptors = objectDescriptor.fields;
    if (fieldDescriptors != nullptr) {
        while (fieldDescriptors->name) {
            const char* name = fieldDescriptors->name;
            const char* type = fieldDescriptors->type;
            auto propertyName = STRING_LITERAL(name);
            auto typeCpp = std::string(std::string_view(type));
            for (const auto& it : replacementPairs) {
                Valdi::stringReplace(typeCpp, it.first, it.second);
            }

            auto propertySchema = Valdi::ValueSchema::parse(typeCpp);
            if (!propertySchema) {
                return propertySchema.error().rethrow(
                    STRING_FORMAT("While parsing property '{}' of {}", propertyName, className));
            }

            properties.emplace_back(Valdi::ClassPropertySchema(propertyName, propertySchema.value()));

            fieldDescriptors++;
        }
    }

    switch (objectDescriptor.objectType) {
        case SwiftValdiMarshallableObjectRegistry_ObjectType_Class:
            return Valdi::ValueSchema::cls(className, false, properties.data(), properties.size());
        case SwiftValdiMarshallableObjectRegistry_ObjectType_Protocol:
            return Valdi::ValueSchema::cls(className, true, properties.data(), properties.size());
        case SwiftValdiMarshallableObjectRegistry_ObjectType_Function:
            return Valdi::ValueSchema::cls(className, false, properties.data(), properties.size());
        case SwiftValdiMarshallableObjectRegistry_ObjectType_Untyped:
            return Valdi::ValueSchema::untyped();
    }
}

SwiftValdiMarshallableObjectRegistry::SwiftValdiMarshallableObjectRegistry(
    Valdi::Ref<Valdi::ValueSchemaRegistry> valueSchemaRegistry)
    : _schemaRegistry(valueSchemaRegistry), _valueSchemaTypeResolver(_schemaRegistry.get()) {}

std::unique_lock<std::recursive_mutex> SwiftValdiMarshallableObjectRegistry::lock() const {
    return _schemaRegistry->lock();
}
Valdi::Result<Valdi::Void> SwiftValdiMarshallableObjectRegistry::setActiveSchemaOfClassInMarshaller(
    const Valdi::StringBox& className, Valdi::Marshaller* marshaller) {
    auto schema = getClassSchema(className);
    if (!schema) {
        return schema.error();
    }
    marshaller->setCurrentSchema(Valdi::ValueSchema::cls(schema.value()));
    return Valdi::Void();
}

Valdi::Result<Valdi::ValueSchema> SwiftValdiMarshallableObjectRegistry::resolveSchemaForIdentifier(
    Valdi::ValueSchemaRegistrySchemaIdentifier identifier) {
    auto schemaAndKey = _schemaRegistry->getSchemaAndKeyForIdentifier(identifier);
    if (!schemaAndKey) {
        return Valdi::Error(STRING_FORMAT("Schema not found for identifier '{}'", identifier));
    }

    bool didChange = false;
    auto resolvedSchema = _valueSchemaTypeResolver.resolveTypeReferences(
        schemaAndKey.value().schema, Valdi::ValueSchemaRegistryResolveType::Schema, &didChange);
    if (!resolvedSchema) {
        return resolvedSchema.error();
    }

    if (didChange && _valueSchemaTypeResolver.getRegistry() != nullptr) {
        _valueSchemaTypeResolver.getRegistry()->updateSchemaIfKeyExists(schemaAndKey.value().schemaKey,
                                                                        resolvedSchema.value());
    }
    return resolvedSchema.value();
}

void SwiftValdiMarshallableObjectRegistry::flushPendingUnresolvedSchemas() {
    while (!_pendingUnresolvedSchemaIds.empty()) {
        auto identifier = _pendingUnresolvedSchemaIds.front();
        _pendingUnresolvedSchemaIds.pop_front();
        auto resolvedSchema = resolveSchemaForIdentifier(identifier);
        if (!resolvedSchema) {
            _pendingUnresolvedSchemaIds.push_front(identifier);
            return;
        }
    }
}

Valdi::Result<Valdi::ValueSchemaRegistrySchemaIdentifier> SwiftValdiMarshallableObjectRegistry::registerClass(
    const Valdi::StringBox& className, const SwiftValdiMarshallableObjectRegistry_ObjectDescriptor& descriptor) {
    auto schema = schemaFromObjectDescriptor(className, descriptor);
    if (!schema) {
        return schema.moveError();
    }
    auto identifier = _schemaRegistry->registerSchema(schema.value());
    _schemaIdentifierByClass.try_emplace(className, identifier);

    auto resolvedSchema = resolveSchemaForIdentifier(identifier);
    if (!resolvedSchema) {
        _pendingUnresolvedSchemaIds.push_back(identifier);
    } else {
        flushPendingUnresolvedSchemas();
    }
    return identifier;
}

Valdi::Result<Valdi::ValueSchemaRegistrySchemaIdentifier> SwiftValdiMarshallableObjectRegistry::registerEnum(
    const Valdi::StringBox& enumName,
    const Valdi::ValueSchema& caseSchema,
    const Valdi::Ref<Valdi::ValueArray> enumValues) {
    Valdi::SmallVector<Valdi::EnumCaseSchema, 16> cases;
    int caseCount = enumValues->size();
    cases.reserve(caseCount);
    for (int i = 0; i < caseCount; i++) {
        cases.emplace_back(STRING_FORMAT("{}", i), (*enumValues)[i]);
    }
    auto schema = Valdi::ValueSchema::enumeration(enumName, caseSchema, cases.data(), cases.size());
    auto schemaKey = Valdi::ValueSchema::typeReference(Valdi::ValueSchemaTypeReference::named(enumName));
    auto identifier = _schemaRegistry->registerSchema(schemaKey, schema);

    _schemaIdentifierByClass.try_emplace(enumName, identifier);
    return identifier;
}

Valdi::Result<Valdi::Ref<Valdi::ClassSchema>> SwiftValdiMarshallableObjectRegistry::getClassSchema(
    const Valdi::StringBox& className) const {
    auto guard = lock();
    const auto& it = _schemaIdentifierByClass.find(className);
    if (it == _schemaIdentifierByClass.end()) {
        return Valdi::Error(STRING_FORMAT("Class schema not found for '{}'", className));
    }
    auto schema = _schemaRegistry->getSchemaForIdentifier(it->second);
    return schema.getClassRef();
}
} // namespace ValdiSwift