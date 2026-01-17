//
//  JavaObjectClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/17/23.
//

#include "valdi/android/JavaObjectClassDelegate.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"

namespace ValdiAndroid {

JavaObjectClassDelegate::JavaObjectClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                 jclass cls,
                                                 const JavaMethodBase& constructor,
                                                 size_t fieldsCount)
    : JavaClassDelegate(identifier, cls), _constructor(constructor), _fieldsCount(fieldsCount) {
    Valdi::InlineContainerAllocator<JavaObjectClassDelegate, AnyJavaField> allocator;
    allocator.constructContainerEntries(this, fieldsCount);
}

JavaObjectClassDelegate::~JavaObjectClassDelegate() {
    Valdi::InlineContainerAllocator<JavaObjectClassDelegate, AnyJavaField> allocator;
    allocator.deallocate(this, _fieldsCount);
}

JavaValue JavaObjectClassDelegate::newObject(const JavaValue* propertyValues,
                                             Valdi::ExceptionTracker& exceptionTracker) {
    return JavaValue::makeObject(getClass().newObjectUntyped(_constructor, propertyValues, _fieldsCount));
}

JavaValue JavaObjectClassDelegate::getProperty(const JavaValue& object,
                                               size_t propertyIndex,
                                               Valdi::ExceptionTracker& exceptionTracker) {
    const auto& field = getField(propertyIndex);
    return field.getFieldValue(object.getObject());
}

Valdi::Ref<Valdi::ValueTypedProxyObject> JavaObjectClassDelegate::newProxy(
    const JavaValue& object,
    const Valdi::Ref<Valdi::ValueTypedObject>& typedObject,
    Valdi::ExceptionTracker& exceptionTracker) {
    // Only implemented in JavaInterfaceClassDelegate. We don't make proxy for non interface classes.
    return nullptr;
}

void JavaObjectClassDelegate::setField(size_t index, const AnyJavaField& field) {
    Valdi::InlineContainerAllocator<JavaObjectClassDelegate, AnyJavaField> allocator;
    allocator.getContainerStartPtr(this)[index] = field;
}

const AnyJavaField& JavaObjectClassDelegate::getField(size_t index) const {
    Valdi::InlineContainerAllocator<JavaObjectClassDelegate, AnyJavaField> allocator;
    return allocator.getContainerStartPtr(this)[index];
}

Valdi::Ref<JavaObjectClassDelegate> JavaObjectClassDelegate::make(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                                  jclass cls,
                                                                  const JavaMethodBase& constructor,
                                                                  size_t fieldsCount) {
    Valdi::InlineContainerAllocator<JavaObjectClassDelegate, AnyJavaField> allocator;
    return allocator.allocate(fieldsCount, identifier, cls, constructor, fieldsCount);
}

} // namespace ValdiAndroid
