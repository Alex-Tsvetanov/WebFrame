//
//  JavaValue.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 8/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/jni/JavaValue.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fmt/format.h>

namespace ValdiAndroid {

// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)

JavaValue::JavaValue() : _type(JavaValueType::Pointer), _refType(JavaValueRefType::None) {
    _value.l = nullptr;
}

JavaValue::JavaValue(const JavaValue& other) : _value(other._value), _type(other._type), _refType(other._refType) {
    acquireRef();
}

JavaValue::JavaValue(JavaValue&& other) noexcept : _value(other._value), _type(other._type), _refType(other._refType) {
    other._value.l = nullptr;
    other._refType = JavaValueRefType::None;
}

JavaValue::~JavaValue() {
    releaseRef();
}

JavaValue& JavaValue::operator=(const JavaValue& other) {
    if (this != &other) {
        releaseRef();

        _value = other._value;
        _type = other._type;
        _refType = other._refType;

        acquireRef();
    }

    return *this;
}

JavaValue& JavaValue::operator=(JavaValue&& other) noexcept {
    if (this != &other) {
        releaseRef();

        _value = other._value;
        _type = other._type;
        _refType = other._refType;

        other._value.l = nullptr;
        other._refType = JavaValueRefType::None;
    }

    return *this;
}

jobject JavaValue::releaseObject() {
    auto* object = getObject();
    _refType = JavaValueRefType::None;
    return object;
}

void JavaValue::acquireRef() {
    switch (_refType) {
        case JavaValueRefType::None:
            break;
        case JavaValueRefType::Local:
            _value.l = JavaEnv::getUnsafeEnv()->NewLocalRef(getObject());
            break;
        case JavaValueRefType::Global:
            _value.l = JavaEnv::getUnsafeEnv()->NewGlobalRef(getObject());
            break;
    }
}

void JavaValue::releaseRef() {
    switch (_refType) {
        case JavaValueRefType::None:
            break;
        case JavaValueRefType::Local:
            JavaEnv::getUnsafeEnv()->DeleteLocalRef(getObject());
            _value.l = nullptr;
            break;
        case JavaValueRefType::Global:
            JavaEnv::getUnsafeEnv()->DeleteGlobalRef(getObject());
            _value.l = nullptr;
            break;
    }
}

jvalue JavaValue::getValue() const {
    return _value;
}

JavaValueType JavaValue::getType() const {
    return _type;
}

JavaValueRefType JavaValue::getRefType() const {
    return _refType;
}

jboolean JavaValue::getBoolean() const {
    SC_ASSERT(_type == JavaValueType::Boolean);
    return _value.z;
}

jbyte JavaValue::getByte() const {
    SC_ASSERT(_type == JavaValueType::Byte);
    return _value.b;
}

jchar JavaValue::getChar() const {
    SC_ASSERT(_type == JavaValueType::Char);
    return _value.c;
}

jshort JavaValue::getShort() const {
    SC_ASSERT(_type == JavaValueType::Short);
    return _value.s;
}

jint JavaValue::getInt() const {
    SC_ASSERT(_type == JavaValueType::Int);
    return _value.i;
}

jlong JavaValue::getLong() const {
    SC_ASSERT(_type == JavaValueType::Long);
    return _value.j;
}

jfloat JavaValue::getFloat() const {
    SC_ASSERT(_type == JavaValueType::Float);
    return _value.f;
}

jdouble JavaValue::getDouble() const {
    SC_ASSERT(_type == JavaValueType::Double);
    return _value.d;
}

jobject JavaValue::getObject() const {
    SC_ASSERT(_type == JavaValueType::Object);
    return _value.l;
}

void* JavaValue::getPointer() const {
    SC_ASSERT(_type == JavaValueType::Pointer);
    return _value.l;
}

jstring JavaValue::getString() const {
    return reinterpret_cast<jstring>(getObject());
}

jobjectArray JavaValue::getObjectArray() const {
    return reinterpret_cast<jobjectArray>(getObject());
}

jbyteArray JavaValue::getByteArray() const {
    return reinterpret_cast<jbyteArray>(getObject());
}

bool JavaValue::isNull() const {
    return _value.l == nullptr;
}

JavaValue JavaValue::makeBoolean(jboolean z) {
    jvalue v;
    v.z = z;
    return JavaValue(v, JavaValueType::Boolean, JavaValueRefType::None);
}

JavaValue JavaValue::makeByte(jbyte b) {
    jvalue v;
    v.b = b;
    return JavaValue(v, JavaValueType::Byte, JavaValueRefType::None);
}

JavaValue JavaValue::makeChar(jchar c) {
    jvalue v;
    v.c = c;
    return JavaValue(v, JavaValueType::Char, JavaValueRefType::None);
}

JavaValue JavaValue::makeShort(jshort s) {
    jvalue v;
    v.s = s;
    return JavaValue(v, JavaValueType::Short, JavaValueRefType::None);
}

JavaValue JavaValue::makeInt(jint i) {
    jvalue v;
    v.i = i;
    return JavaValue(v, JavaValueType::Int, JavaValueRefType::None);
}

JavaValue JavaValue::makeLong(jlong j) {
    jvalue v;
    v.j = j;
    return JavaValue(v, JavaValueType::Long, JavaValueRefType::None);
}

JavaValue JavaValue::makeFloat(jfloat f) {
    jvalue v;
    v.f = f;
    return JavaValue(v, JavaValueType::Float, JavaValueRefType::None);
}

JavaValue JavaValue::makeDouble(jdouble d) {
    jvalue v;
    v.d = d;
    return JavaValue(v, JavaValueType::Double, JavaValueRefType::None);
}

JavaValue JavaValue::unsafeMakeObject(jobject l) {
    jvalue v;
    v.l = l;
    return JavaValue(v, JavaValueType::Object, JavaValueRefType::None);
}

JavaValue JavaValue::makeObject(djinni::LocalRef<jobject> object) {
    jvalue v;
    v.l = object.release();
    return JavaValue(v, JavaValueType::Object, JavaValueRefType::Local);
}

JavaValue JavaValue::makeObject(djinni::LocalRef<jstring> str) {
    jvalue v;
    v.l = str.release();
    return JavaValue(v, JavaValueType::Object, JavaValueRefType::Local);
}

JavaValue JavaValue::makeObject(djinni::LocalRef<jbyteArray> byteArray) {
    jvalue v;
    v.l = byteArray.release();
    return JavaValue(v, JavaValueType::Object, JavaValueRefType::Local);
}

JavaValue JavaValue::makeObject(djinni::LocalRef<jobjectArray> objectArray) {
    jvalue v;
    v.l = objectArray.release();
    return JavaValue(v, JavaValueType::Object, JavaValueRefType::Local);
}

JavaValue JavaValue::makeObject(djinni::GlobalRef<jobject> object) {
    jvalue v;
    v.l = object.release();
    return JavaValue(v, JavaValueType::Object, JavaValueRefType::Global);
}

JavaValue JavaValue::makePointer(void* pointer) {
    jvalue v;
    v.l = reinterpret_cast<jobject>(pointer);
    return JavaValue(v, JavaValueType::Pointer, JavaValueRefType::None);
}

// NOLINTEND(cppcoreguidelines-pro-type-union-access)

} // namespace ValdiAndroid
