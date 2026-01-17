//
//  SCValdiObjCValue.m
//  valdi_core-ios
//
//  Created by Simon Corsin on 1/31/23.
//

#import "SCValdiObjCValue.h"
#import "valdi_core/cpp/Constants.hpp"

namespace ValdiIOS {

static inline void assertFieldType(SCValdiFieldValueType type, SCValdiFieldValueType expectedType) {
    if (VALDI_UNLIKELY(type != expectedType)) {
        std::abort();
    }
}

ObjCValue::ObjCValue(): _value(SCValdiFieldValueMakeNull()), _type(SCValdiFieldValueTypeVoid) {}

ObjCValue::ObjCValue(const SCValdiFieldValue &value, SCValdiFieldValueType type): _value(value), _type(type) {
    safeRetain();
}

ObjCValue::ObjCValue(const ObjCValue &other): _value(other._value), _type(other._type) {
    safeRetain();
}

ObjCValue::ObjCValue(ObjCValue &&other): _value(other._value), _type(other._type) {
    other._value = SCValdiFieldValueMakeNull();
    other._type = SCValdiFieldValueTypeVoid;
}

ObjCValue::~ObjCValue() {
    safeRelease();
}

ObjCValue& ObjCValue::operator=(ObjCValue&& other) noexcept {
    if (this != &other) {
        safeRelease();

        _value = other._value;
        _type = other._type;

        other._value = SCValdiFieldValueMakeNull();
        other._type = SCValdiFieldValueTypeVoid;
    }

    return *this;
}

ObjCValue& ObjCValue::operator=(const ObjCValue& other) noexcept {
    if (this != &other) {
        safeRelease();

        _value = other._value;
        _type = other._type;

        safeRetain();
    }

    return *this;
}

int32_t ObjCValue::getInt() const {
    assertFieldType(_type, SCValdiFieldValueTypeInt);
    return _value.i;
}

int64_t ObjCValue::getLong() const {
    assertFieldType(_type, SCValdiFieldValueTypeLong);
    return _value.l;
}

bool ObjCValue::getBool() const {
    assertFieldType(_type, SCValdiFieldValueTypeBool);
    return _value.b;
}

double ObjCValue::getDouble() const {
    assertFieldType(_type, SCValdiFieldValueTypeDouble);
    return _value.d;
}

id ObjCValue::getObject() const {
    assertFieldType(_type, SCValdiFieldValueTypeObject);
    return (__bridge id)_value.o;
}

const void * ObjCValue::getPtr() const {
    assertFieldType(_type, SCValdiFieldValueTypePtr);
    return _value.o;
}

bool ObjCValue::isPtr() const {
    return _type == SCValdiFieldValueTypePtr;
}

bool ObjCValue::isVoid() const {
    return _type == SCValdiFieldValueTypeVoid;
}

SCValdiFieldValue ObjCValue::getAsParameterValue(SCValdiFieldValueType expectedType) const {
    // Don't fail the assert if the type is ptr and we expect object.
    if (!(expectedType == SCValdiFieldValueTypeObject && _type == SCValdiFieldValueTypePtr)) {
        assertFieldType(_type, expectedType);
    }

    return _value;
}

SCValdiFieldValue ObjCValue::getAsReturnValue(SCValdiFieldValueType expectedType) const {
    // Don't fail the assert if the type is ptr and we expect object.
    if (!(expectedType == SCValdiFieldValueTypeObject && _type == SCValdiFieldValueTypePtr)) {
        assertFieldType(_type, expectedType);
    }

    if (_type == SCValdiFieldValueTypeObject) {
        return SCValdiFieldValueMakeObject((__bridge id)_value.o);
    } else {
        return _value;
    }
}

ObjCValue ObjCValue::makeInt(int32_t i) {
    return ObjCValue(SCValdiFieldValueMakeInt(i), SCValdiFieldValueTypeInt);
}

ObjCValue ObjCValue::makeLong(int64_t l) {
    return ObjCValue(SCValdiFieldValueMakeLong(l), SCValdiFieldValueTypeLong);
}

ObjCValue ObjCValue::makeBool(bool b) {
    return ObjCValue(SCValdiFieldValueMakeBool(b), SCValdiFieldValueTypeBool);
}

ObjCValue ObjCValue::makeDouble(double d) {
    return ObjCValue(SCValdiFieldValueMakeDouble(d), SCValdiFieldValueTypeDouble);
}

ObjCValue ObjCValue::makeObject(__unsafe_unretained id o) {
    return ObjCValue(SCValdiFieldValueMakeUnretainedObject(o), SCValdiFieldValueTypeObject);
}

ObjCValue ObjCValue::makeUnretainedObject(__unsafe_unretained id o) {
    return ObjCValue(SCValdiFieldValueMakeUnretainedObject(o), SCValdiFieldValueTypeObject);
}

ObjCValue ObjCValue::makePtr(const void *ptr) {
    return ObjCValue(SCValdiFieldValueMakePtr(ptr), SCValdiFieldValueTypePtr);
}

ObjCValue ObjCValue::makeNull() {
    return ObjCValue(SCValdiFieldValueMakeNull(), SCValdiFieldValueTypeObject);
}

inline void ObjCValue::safeRetain() {
    if (_type == SCValdiFieldValueTypeObject && _value.o != nullptr) {
        CFRetain(_value.o);
    }
}

inline void ObjCValue::safeRelease() {
    if (_type == SCValdiFieldValueTypeObject && _value.o != nullptr) {
        CFRelease(_value.o);
    }
}

};

