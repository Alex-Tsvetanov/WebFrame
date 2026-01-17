//
//  SwiftValdiMarshallerRegistry.h
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//
#pragma once

#include "valdi_core/cpp/Schema/ValueSchemaRegistry.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMarshallerRegistry.hpp"
#include "valdi_core/ios/swift/cpp/SwiftValdiMarshallableObjectRegistry.h"

namespace ValdiSwift {

class SwiftValdiMarshallableObjectRegistry {
public:
    SwiftValdiMarshallableObjectRegistry(
        Valdi::Ref<Valdi::ValueSchemaRegistry> valueSchemaRegistry = Valdi::makeShared<Valdi::ValueSchemaRegistry>());
    Valdi::Result<Valdi::Void> setActiveSchemaOfClassInMarshaller(const Valdi::StringBox& className,
                                                                  Valdi::Marshaller* marshaller);

    Valdi::Result<Valdi::ValueSchemaRegistrySchemaIdentifier> registerClass(
        const Valdi::StringBox& className, const SwiftValdiMarshallableObjectRegistry_ObjectDescriptor& descriptor);

    Valdi::Result<Valdi::ValueSchemaRegistrySchemaIdentifier> registerEnum(
        const Valdi::StringBox& enumName,
        const Valdi::ValueSchema& caseSchema,
        const Valdi::Ref<Valdi::ValueArray> enumValues);

    Valdi::Result<Valdi::Ref<Valdi::ClassSchema>> getClassSchema(const Valdi::StringBox& className) const;

private:
    Valdi::Result<Valdi::ValueSchema> resolveSchemaForIdentifier(Valdi::ValueSchemaRegistrySchemaIdentifier identifier);
    void flushPendingUnresolvedSchemas();
    std::unique_lock<std::recursive_mutex> lock() const;

    Valdi::Ref<Valdi::ValueSchemaRegistry> _schemaRegistry;
    Valdi::ValueSchemaTypeResolver _valueSchemaTypeResolver;
    Valdi::FlatMap<Valdi::StringBox, Valdi::ValueSchemaRegistrySchemaIdentifier> _schemaIdentifierByClass;
    std::deque<Valdi::ValueSchemaRegistrySchemaIdentifier> _pendingUnresolvedSchemaIds;
};

} // namespace ValdiSwift