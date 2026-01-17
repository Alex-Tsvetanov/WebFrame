//
//  JavaInterfaceClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/17/23.
//

#include "valdi/android/JavaInterfaceClassDelegate.hpp"
#include "valdi/android/ValdiMarshallableObjectDescriptorJavaClass.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"

namespace ValdiAndroid {

JavaInterfaceClassDelegate::JavaInterfaceClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                       jclass cls,
                                                       jclass interfaceCls,
                                                       const JavaMethodBase& constructor,
                                                       size_t methodsCount)
    : JavaClassDelegate(identifier, cls),
      _interfaceClass(JavaEnv(), interfaceCls),
      _constructor(constructor),
      _methodsCount(methodsCount) {
    Valdi::InlineContainerAllocator<JavaInterfaceClassDelegate, JavaInterfaceClassDelegateMethod> allocator;
    allocator.constructContainerEntries(this, methodsCount);
}

JavaInterfaceClassDelegate::~JavaInterfaceClassDelegate() {
    Valdi::InlineContainerAllocator<JavaInterfaceClassDelegate, JavaInterfaceClassDelegateMethod> allocator;
    allocator.deallocate(this, _methodsCount);
}

JavaValue JavaInterfaceClassDelegate::newObject(const JavaValue* propertyValues,
                                                Valdi::ExceptionTracker& exceptionTracker) {
    return JavaValue::makeObject(getClass().newObjectUntyped(_constructor, propertyValues, _methodsCount));
}

bool JavaInterfaceClassDelegate::objectImplementsMethod(const JavaValue& object,
                                                        const IndirectJavaGlobalRef& method) const {
    auto reflectedMethod = method.get();
    std::initializer_list<JavaValue> parameters = {JavaValue::unsafeMakeObject(object.getObject()),
                                                   JavaValue::unsafeMakeObject(reflectedMethod.get())};
    const auto& objectDescriptorClass = ValdiMarshallableObjectDescriptorJavaClass::get();

    return objectDescriptorClass.objectImplementsMethodMethod
               .call(objectDescriptorClass.cls.getClass(), parameters.size(), parameters.begin())
               .getBoolean() != 0;
}

JavaValue JavaInterfaceClassDelegate::getProperty(const JavaValue& object,
                                                  size_t propertyIndex,
                                                  Valdi::ExceptionTracker& exceptionTracker) {
    Valdi::InlineContainerAllocator<JavaInterfaceClassDelegate, JavaInterfaceClassDelegateMethod> allocator;
    auto& method = allocator.getContainerStartPtr(this)[propertyIndex];

    if (method.isPropertyLike) {
        return method.method.call(object.getObject(), 0, nullptr);
    } else {
        if (method.isOptional && !objectImplementsMethod(object, method.javaMethodRef)) {
            return JavaValue::makePointer(nullptr);
        }

        // We pass the AnyJavaMethod pointer when resolving interface properties.
        // It will be unwrapped by JavaMethodClassDelegate
        return JavaValue::makePointer(&method);
    }
}

void JavaInterfaceClassDelegate::setMethod(
    size_t index, const AnyJavaMethod& method, jobject javaMethod, bool isOptional, bool isPropertyLike) {
    Valdi::InlineContainerAllocator<JavaInterfaceClassDelegate, JavaInterfaceClassDelegateMethod> allocator;
    auto& entry = allocator.getContainerStartPtr(this)[index];
    entry.method = method;
    if (isOptional && !isPropertyLike) {
        entry.javaMethodRef = IndirectJavaGlobalRef::make(javaMethod, "Optional Java Method");
    } else {
        entry.javaMethodRef = IndirectJavaGlobalRef();
    }
    entry.isOptional = isOptional;
    entry.isPropertyLike = isPropertyLike;
}

Valdi::Ref<Valdi::ValueTypedProxyObject> JavaInterfaceClassDelegate::newProxy(
    const JavaValue& object,
    const Valdi::Ref<Valdi::ValueTypedObject>& typedObject,
    Valdi::ExceptionTracker& exceptionTracker) {
    return newJavaProxy(object.getObject(), typedObject);
}

Valdi::Ref<JavaInterfaceClassDelegate> JavaInterfaceClassDelegate::make(
    Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
    jclass cls,
    jclass interfaceCls,
    const JavaMethodBase& constructor,
    size_t methodsCount) {
    Valdi::InlineContainerAllocator<JavaInterfaceClassDelegate, JavaInterfaceClassDelegateMethod> allocator;
    return allocator.allocate(methodsCount, identifier, cls, interfaceCls, constructor, methodsCount);
}

} // namespace ValdiAndroid
