//
//  JavaInterfaceClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/17/23.
//

#pragma once

#include "valdi/android/JavaClassDelegate.hpp"
#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"

namespace ValdiAndroid {

struct JavaInterfaceClassDelegateMethod {
    AnyJavaMethod method;
    // Will only be set for optional methods
    IndirectJavaGlobalRef javaMethodRef;
    bool isOptional = false;
    // Whether the method should be treated like a property
    bool isPropertyLike = false;
};

/**
 Concrete implementation of JavaClassDelegate which integrates the PlatformObjectClassDelegate API.
 Allows the ValueMarshallerRegistry to create and retrieve properties from Java objects.
 */
class JavaInterfaceClassDelegate : public Valdi::PlatformObjectClassDelegate<JavaValue>, public JavaClassDelegate {
public:
    ~JavaInterfaceClassDelegate() override;

    JavaValue newObject(const JavaValue* propertyValues, Valdi::ExceptionTracker& exceptionTracker) final;

    JavaValue getProperty(const JavaValue& object,
                          size_t propertyIndex,
                          Valdi::ExceptionTracker& exceptionTracker) final;

    Valdi::Ref<Valdi::ValueTypedProxyObject> newProxy(const JavaValue& object,
                                                      const Valdi::Ref<Valdi::ValueTypedObject>& typedObject,
                                                      Valdi::ExceptionTracker& exceptionTracker) final;

    void setMethod(size_t index, const AnyJavaMethod& method, jobject javaMethod, bool isOptional, bool isPropertyLike);

    static Valdi::Ref<JavaInterfaceClassDelegate> make(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                       jclass cls,
                                                       jclass interfaceCls,
                                                       const JavaMethodBase& constructor,
                                                       size_t methodsCount);

private:
    friend Valdi::InlineContainerAllocator<JavaInterfaceClassDelegate, JavaInterfaceClassDelegateMethod>;
    JavaClass _interfaceClass;
    JavaMethodBase _constructor;
    size_t _methodsCount;

    JavaInterfaceClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                               jclass cls,
                               jclass interfaceCls,
                               const JavaMethodBase& constructor,
                               size_t methodsCount);

    bool objectImplementsMethod(const JavaValue& object, const IndirectJavaGlobalRef& method) const;
};

} // namespace ValdiAndroid
