//
//  JavaClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/14/23.
//

#pragma once

#include "valdi_core/cpp/Schema/ValueSchemaRegistry.hpp"
#include "valdi_core/cpp/Utils/PlatformValueDelegate.hpp"
#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaField.hpp"
#include "valdi_core/jni/JavaMethod.hpp"
#include "valdi_core/jni/JavaValue.hpp"

namespace Valdi {
template<typename ValueType>
class ValueMarshaller;

}

namespace ValdiAndroid {

/**
 Base class for enum, class and interface delegates.
 */
class JavaClassDelegate {
public:
    JavaClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier, jclass cls);
    virtual ~JavaClassDelegate();

    Valdi::ValueSchemaRegistrySchemaIdentifier getIdentifier() const;
    const JavaClass& getClass() const;

    Valdi::ValueMarshaller<JavaValue>* getValueMarshaller() const;
    void setValueMarshaller(Valdi::ValueMarshaller<JavaValue>* valueMarshaller);

    Valdi::ClassSchema* getSchema() const;
    void setSchema(Valdi::ClassSchema* schema);

private:
    Valdi::ValueSchemaRegistrySchemaIdentifier _identifier;
    JavaClass _class;
    Valdi::ValueMarshaller<JavaValue>* _valueMarshaller = nullptr;
    Valdi::ClassSchema* _schema = nullptr;
};

} // namespace ValdiAndroid
