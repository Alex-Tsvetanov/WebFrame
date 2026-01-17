//
//  ValueTypedObject.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 2/1/23.
//

#include "valdi_core/cpp/Utils/ValueTypedObject.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"
#include <boost/functional/hash.hpp>

namespace Valdi {

ValueTypedObjectIterator::ValueTypedObjectIterator(const ClassPropertySchema* propertyNames,
                                                   const Value* propertyValues)
    : _propertyNames(propertyNames), _propertyValues(propertyValues) {}

ValueTypedObjectIterator& ValueTypedObjectIterator::operator++() {
    _propertyNames++;
    _propertyValues++;
    return *this;
}

ValueTypedObjectProperty ValueTypedObjectIterator::operator*() const {
    return ValueTypedObjectProperty(_propertyNames->name, *_propertyValues);
}

bool ValueTypedObjectIterator::operator!=(const ValueTypedObjectIterator& other) const {
    return _propertyValues != other._propertyValues;
}

ValueTypedObject::ValueTypedObject(const Ref<ClassSchema>& classSchema) : _classSchema(classSchema) {
    InlineContainerAllocator<ValueTypedObject, Value> allocator;
    allocator.constructContainerEntries(this, getPropertiesSize());
}

ValueTypedObject::~ValueTypedObject() {
    InlineContainerAllocator<ValueTypedObject, Value> allocator;
    allocator.deallocate(this, getPropertiesSize());
}

const StringBox& ValueTypedObject::getClassName() const {
    return _classSchema->getClassName();
}

const Ref<ClassSchema>& ValueTypedObject::getSchema() const {
    return _classSchema;
}

size_t ValueTypedObject::getPropertiesSize() const {
    return _classSchema->getPropertiesSize();
}

const Value* ValueTypedObject::getProperties() const {
    InlineContainerAllocator<ValueTypedObject, Value> allocator;
    return allocator.getContainerStartPtr(this);
}

Value* ValueTypedObject::getProperties() {
    InlineContainerAllocator<ValueTypedObject, Value> allocator;
    return allocator.getContainerStartPtr(this);
}

const Value& ValueTypedObject::getProperty(size_t index) const {
    SC_ASSERT(index < getPropertiesSize());
    return getProperties()[index];
}

void ValueTypedObject::setProperty(size_t index, const Value& value) {
    SC_ASSERT(index < getPropertiesSize());
    getProperties()[index] = value;
}

void ValueTypedObject::setProperty(size_t index, Value&& value) {
    SC_ASSERT(index < getPropertiesSize());
    getProperties()[index] = std::move(value);
}

const Value& ValueTypedObject::getPropertyForName(const StringBox& propertyName) const {
    auto propertyIndex = _classSchema->getPropertyIndexForName(propertyName);
    if (!propertyIndex) {
        return Value::undefinedRef();
    }

    return getProperty(propertyIndex.value());
}

bool ValueTypedObject::setPropertyForName(const StringBox& propertyName, const Value& value) {
    auto propertyIndex = _classSchema->getPropertyIndexForName(propertyName);
    if (!propertyIndex) {
        return false;
    }

    getProperties()[propertyIndex.value()] = value;
    return true;
}

size_t ValueTypedObject::hash() const {
    auto size = getPropertiesSize();

    size_t hash = _classSchema->hash();
    const auto* properties = getProperties();
    for (size_t i = 0; i < size; i++) {
        boost::hash_combine(hash, properties[i].hash());
    }

    return hash;
}

ValueTypedObjectIterator ValueTypedObject::begin() const {
    return ValueTypedObjectIterator(_classSchema->getProperties(), getProperties());
}

ValueTypedObjectIterator ValueTypedObject::end() const {
    return ValueTypedObjectIterator(_classSchema->getProperties() + getPropertiesSize(),
                                    getProperties() + getPropertiesSize());
}

bool ValueTypedObject::operator==(const ValueTypedObject& other) const {
    if (_classSchema != other._classSchema) {
        return false;
    }

    auto size = getPropertiesSize();
    const auto* properties = getProperties();
    const auto* otherProperties = other.getProperties();
    for (size_t i = 0; i < size; i++) {
        if (properties[i] != otherProperties[i]) {
            return false;
        }
    }

    return true;
}

bool ValueTypedObject::operator!=(const ValueTypedObject& other) const {
    return !(*this == other);
}

Ref<ValueMap> ValueTypedObject::toValueMap(bool ignoreUndefinedValues) const {
    auto out = makeShared<ValueMap>();
    out->reserve(getPropertiesSize());

    for (const auto& property : *this) {
        if (!ignoreUndefinedValues || !property.value.isUndefined()) {
            (*out)[property.name] = property.value;
        }
    }

    return out;
}

Ref<ValueTypedObject> ValueTypedObject::make(const Ref<ClassSchema>& classSchema) {
    InlineContainerAllocator<ValueTypedObject, Value> allocator;
    return allocator.allocate(classSchema->getPropertiesSize(), classSchema);
}

} // namespace Valdi
