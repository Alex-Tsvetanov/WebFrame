//
//  ValdiMarshallableObjectDescriptorJavaClass.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/22/23.
//

#pragma once

#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaField.hpp"
#include "valdi_core/jni/JavaMethod.hpp"

namespace ValdiAndroid {

struct ValdiMarshallableObjectDescriptorJavaClass {
    JavaClass cls;
    AnyJavaMethod getDescriptorForClassMethod;
    AnyJavaMethod objectImplementsMethodMethod;
    AnyJavaField typeField;
    AnyJavaField schemaField;
    AnyJavaField proxyClassField;
    AnyJavaField typeReferencesField;
    AnyJavaField propertyReplacementsField;

    static const ValdiMarshallableObjectDescriptorJavaClass& get();

private:
    explicit ValdiMarshallableObjectDescriptorJavaClass(JavaClass&& cls);
    static ValdiMarshallableObjectDescriptorJavaClass make();
};

} // namespace ValdiAndroid
