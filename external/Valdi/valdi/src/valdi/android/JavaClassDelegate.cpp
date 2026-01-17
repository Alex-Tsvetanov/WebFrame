//
//  JavaClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/14/23.
//

#include "valdi/android/JavaClassDelegate.hpp"

namespace ValdiAndroid {

JavaClassDelegate::JavaClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier, jclass cls)
    : _identifier(identifier), _class(JavaEnv(), cls) {}

JavaClassDelegate::~JavaClassDelegate() = default;

Valdi::ValueSchemaRegistrySchemaIdentifier JavaClassDelegate::getIdentifier() const {
    return _identifier;
}

const JavaClass& JavaClassDelegate::getClass() const {
    return _class;
}

Valdi::ValueMarshaller<JavaValue>* JavaClassDelegate::getValueMarshaller() const {
    return _valueMarshaller;
}

void JavaClassDelegate::setValueMarshaller(Valdi::ValueMarshaller<JavaValue>* valueMarshaller) {
    _valueMarshaller = valueMarshaller;
}

Valdi::ClassSchema* JavaClassDelegate::getSchema() const {
    return _schema;
}

void JavaClassDelegate::setSchema(Valdi::ClassSchema* schema) {
    _schema = schema;
}

} // namespace ValdiAndroid
