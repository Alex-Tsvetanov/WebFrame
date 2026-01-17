//
//  JavaEnumClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/15/23.
//

#include "valdi/android/JavaEnumClassDelegate.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"

namespace ValdiAndroid {

JavaEnumClassDelegate::JavaEnumClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                             jclass cls,
                                             const Valdi::Ref<Valdi::EnumSchema>& enumSchema)
    : JavaClassDelegate(identifier, cls), _enumSchema(enumSchema) {
    Valdi::InlineContainerAllocator<JavaEnumClassDelegate, IndirectJavaGlobalRef> allocator;
    allocator.constructContainerEntries(this, enumSchema->getCasesSize());
}

JavaEnumClassDelegate::~JavaEnumClassDelegate() {
    Valdi::InlineContainerAllocator<JavaEnumClassDelegate, IndirectJavaGlobalRef> allocator;
    allocator.deallocate(this, _enumSchema->getCasesSize());
}

JavaValue JavaEnumClassDelegate::newEnum(size_t enumCaseIndex,
                                         bool /*asBoxed*/,
                                         Valdi::ExceptionTracker& /*exceptionTracker*/) {
    return getEnumCase(enumCaseIndex);
}

Valdi::Value JavaEnumClassDelegate::enumCaseToValue(const JavaValue& enumeration,
                                                    bool /*isBoxed*/,
                                                    Valdi::ExceptionTracker& exceptionTracker) {
    size_t index = 0;
    auto* jEnumeration = enumeration.getObject();
    for (const auto& enumCase : *_enumSchema) {
        auto javaEnumCase = getEnumCase(index);
        if (JavaEnv::isSameObject(jEnumeration, javaEnumCase.getObject())) {
            return enumCase.value;
        }

        index++;
    }

    exceptionTracker.onError(Valdi::Error("Could not resolve enum value"));
    return Valdi::Value();
}

void JavaEnumClassDelegate::setEnumCase(size_t index, const JavaValue& javaValue) {
    Valdi::InlineContainerAllocator<JavaEnumClassDelegate, IndirectJavaGlobalRef> allocator;
    allocator.getContainerStartPtr(this)[index] = IndirectJavaGlobalRef::make(javaValue.getObject(), "EnumValue");
}

JavaValue JavaEnumClassDelegate::getEnumCase(size_t index) const {
    Valdi::InlineContainerAllocator<JavaEnumClassDelegate, IndirectJavaGlobalRef> allocator;
    return JavaValue::makeObject(allocator.getContainerStartPtr(this)[index].get());
}

Valdi::Ref<JavaEnumClassDelegate> JavaEnumClassDelegate::make(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                              jclass cls,
                                                              const Valdi::Ref<Valdi::EnumSchema>& enumSchema) {
    Valdi::InlineContainerAllocator<JavaEnumClassDelegate, IndirectJavaGlobalRef> allocator;
    return allocator.allocate(enumSchema->getCasesSize(), identifier, cls, enumSchema);
}

} // namespace ValdiAndroid
