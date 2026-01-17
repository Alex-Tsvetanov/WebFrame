//
//  SwiftValdiMarshallableObjectRegistryInterface.cpp
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//

#include "valdi_core/ios/swift/cpp/SwiftValdiMarshallableObjectRegistry.h"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/ios/swift/cpp/SwiftValdiMarshallableObjectRegistryImpl.hpp"
#include "valdi_core/ios/swift/cpp/SwiftValdiMarshallerPrivate.hpp"

SwiftValdiMarshallableObjectRegistryRef SwiftValdiMarshallableObjectRegistry_Create(void* valueSchemaRegistryPtr) {
    Valdi::Ref<Valdi::ValueSchemaRegistry> valueSchemaRegistry(
        reinterpret_cast<Valdi::ValueSchemaRegistry*>(valueSchemaRegistryPtr));
    return new ValdiSwift::SwiftValdiMarshallableObjectRegistry(valueSchemaRegistry);
}

void SwiftValdiMarshallableObjectRegistry_Destroy(SwiftValdiMarshallableObjectRegistryRef marshallerRegistry) {
    delete reinterpret_cast<ValdiSwift::SwiftValdiMarshallableObjectRegistry*>(marshallerRegistry);
}

void SwiftValdiMarshallableObjectRegistry_RegisterClass(
    SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
    const char* className,
    SwiftValdiMarshallableObjectRegistry_ObjectDescriptor descriptor) {
    ValdiSwift::SwiftValdiMarshallableObjectRegistry* registry =
        reinterpret_cast<ValdiSwift::SwiftValdiMarshallableObjectRegistry*>(marshallerRegistry);
    registry->registerClass(STRING_LITERAL(className), descriptor);
}

void SwiftValdiMarshallableObjectRegistry_RegisterStringEnum(SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
                                                             const char* enumName,
                                                             const char* const* enumCases,
                                                             const int32_t numCases) {
    ValdiSwift::SwiftValdiMarshallableObjectRegistry* registry =
        reinterpret_cast<ValdiSwift::SwiftValdiMarshallableObjectRegistry*>(marshallerRegistry);

    auto arrayBuilder = Valdi::ValueArrayBuilder();
    for (int i = 0; i < numCases; ++i) {
        arrayBuilder.append(Valdi::Value(std::string_view(enumCases[i])));
    }
    registry->registerEnum(STRING_LITERAL(enumName), Valdi::ValueSchema::string(), arrayBuilder.build());
}

void SwiftValdiMarshallableObjectRegistry_RegisterIntEnum(SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
                                                          const char* enumName,
                                                          const int32_t* enumCases,
                                                          const int32_t numCases) {
    ValdiSwift::SwiftValdiMarshallableObjectRegistry* registry =
        reinterpret_cast<ValdiSwift::SwiftValdiMarshallableObjectRegistry*>(marshallerRegistry);

    auto arrayBuilder = Valdi::ValueArrayBuilder();
    for (int i = 0; i < numCases; ++i) {
        arrayBuilder.append(Valdi::Value(enumCases[i]));
    }
    registry->registerEnum(STRING_LITERAL(enumName), Valdi::ValueSchema::integer(), arrayBuilder.build());
}

void SwiftValdiMarshallableObjectRegistry_SetActiveSchemeOfClassInMarshaller(
    SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
    const char* className,
    SwiftValdiMarshallerRef marshaller) {
    ValdiSwift::SwiftValdiMarshallableObjectRegistry* registry =
        reinterpret_cast<ValdiSwift::SwiftValdiMarshallableObjectRegistry*>(marshallerRegistry);
    registry->setActiveSchemaOfClassInMarshaller(STRING_LITERAL(className), SwiftValdiMarshaller_Unwrap(marshaller));
}