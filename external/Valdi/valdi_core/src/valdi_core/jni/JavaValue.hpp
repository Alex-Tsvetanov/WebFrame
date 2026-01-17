//
//  JavaValue.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 8/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include <djinni/jni/djinni_support.hpp>
#include <variant>

namespace ValdiAndroid {

/**
 List of JVM types that the JavaValue can hold.
 The ordering should not be changed, as some code relies on it,
 like JavaField.
 */
enum class JavaValueType : uint8_t {
    Boolean,
    Byte,
    Char,
    Short,
    Int,
    Long,
    Float,
    Double,
    Object,
    Pointer,
};

enum class JavaValueRefType : uint8_t { None, Local, Global };

class IndirectJavaGlobalRef;
class UnmanagedIndirectJavaGlobalRef;

class JavaValue {
public:
    JavaValue();
    JavaValue(const JavaValue& other);
    JavaValue(JavaValue&& other) noexcept;
    ~JavaValue();

    JavaValue& operator=(const JavaValue& other);
    JavaValue& operator=(JavaValue&& other) noexcept;

    jvalue getValue() const;
    JavaValueType getType() const;
    JavaValueRefType getRefType() const;

    jboolean getBoolean() const;
    jbyte getByte() const;
    jchar getChar() const;
    jshort getShort() const;
    jint getInt() const;
    jlong getLong() const;
    jfloat getFloat() const;
    jdouble getDouble() const;
    jobject getObject() const;
    jstring getString() const;
    jobjectArray getObjectArray() const;
    jbyteArray getByteArray() const;

    void* getPointer() const;

    bool isNull() const;

    jobject releaseObject();

    static JavaValue makeBoolean(jboolean z);
    static JavaValue makeByte(jbyte b);
    static JavaValue makeChar(jchar c);
    static JavaValue makeShort(jshort s);
    static JavaValue makeInt(jint i);
    static JavaValue makeLong(jlong j);
    static JavaValue makeFloat(jfloat f);
    static JavaValue makeDouble(jdouble d);
    static JavaValue unsafeMakeObject(jobject l);
    static JavaValue makeObject(djinni::LocalRef<jobject> object);
    static JavaValue makeObject(djinni::LocalRef<jstring> str);
    static JavaValue makeObject(djinni::LocalRef<jbyteArray> byteArray);
    static JavaValue makeObject(djinni::LocalRef<jobjectArray> objectArray);
    static JavaValue makeObject(djinni::GlobalRef<jobject> object);
    static JavaValue makePointer(void* pointer);

private:
    constexpr JavaValue(jvalue value, JavaValueType type, JavaValueRefType refType)
        : _value(value), _type(type), _refType(refType) {}

    jvalue _value;
    JavaValueType _type;
    JavaValueRefType _refType;

    void releaseRef();
    void acquireRef();
};

} // namespace ValdiAndroid
