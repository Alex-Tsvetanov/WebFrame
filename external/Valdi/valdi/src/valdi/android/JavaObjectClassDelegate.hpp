//
//  JavaObjectClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/17/23.
//

#pragma once

#include "valdi/android/JavaClassDelegate.hpp"

namespace ValdiAndroid {

/**
 Concrete implementation of JavaClassDelegate which integrates the PlatformObjectClassDelegate API.
 Allows the ValueMarshallerRegistry to create and retrieve properties from Java objects.
 */
class JavaObjectClassDelegate : public Valdi::PlatformObjectClassDelegate<JavaValue>, public JavaClassDelegate {
public:
    ~JavaObjectClassDelegate() override;

    JavaValue newObject(const JavaValue* propertyValues, Valdi::ExceptionTracker& exceptionTracker) final;

    JavaValue getProperty(const JavaValue& object,
                          size_t propertyIndex,
                          Valdi::ExceptionTracker& exceptionTracker) final;

    Valdi::Ref<Valdi::ValueTypedProxyObject> newProxy(const JavaValue& object,
                                                      const Valdi::Ref<Valdi::ValueTypedObject>& typedObject,
                                                      Valdi::ExceptionTracker& exceptionTracker) final;

    void setField(size_t index, const AnyJavaField& field);
    const AnyJavaField& getField(size_t index) const;

    static Valdi::Ref<JavaObjectClassDelegate> make(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                    jclass cls,
                                                    const JavaMethodBase& constructor,
                                                    size_t fieldsCount);

private:
    friend Valdi::InlineContainerAllocator<JavaObjectClassDelegate, AnyJavaField>;
    JavaMethodBase _constructor;
    size_t _fieldsCount;

    JavaObjectClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                            jclass cls,
                            const JavaMethodBase& constructor,
                            size_t fieldsCount);
};

} // namespace ValdiAndroid
