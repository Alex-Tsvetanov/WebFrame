//
//  TypedObject.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 2/1/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

template<typename T, typename ValueType>
struct InlineContainerAllocator;

class ClassSchema;
struct ClassPropertySchema;

struct ValueTypedObjectProperty {
    StringBox name;
    Value value;

    inline ValueTypedObjectProperty(const StringBox& name, const Value& value) : name(name), value(value) {}
};

class ValueTypedObjectIterator {
public:
    ValueTypedObjectIterator(const ClassPropertySchema* propertyNames, const Value* propertyValues);

    ValueTypedObjectIterator& operator++();
    ValueTypedObjectProperty operator*() const;

    bool operator!=(const ValueTypedObjectIterator& other) const;

private:
    const ClassPropertySchema* _propertyNames;
    const Value* _propertyValues;
};

class ValueTypedObject : public SimpleRefCountable {
public:
    ~ValueTypedObject() override;

    const StringBox& getClassName() const;

    const Ref<ClassSchema>& getSchema() const;

    size_t getPropertiesSize() const;

    const Value* getProperties() const;
    Value* getProperties();

    const Value& getProperty(size_t index) const;
    void setProperty(size_t index, const Value& value);
    void setProperty(size_t index, Value&& value);

    const Value& getPropertyForName(const StringBox& propertyName) const;
    bool setPropertyForName(const StringBox& propertyName, const Value& value);

    Ref<ValueMap> toValueMap(bool ignoreUndefinedValues = false) const;

    ValueTypedObjectIterator begin() const;
    ValueTypedObjectIterator end() const;

    size_t hash() const;

    bool operator==(const ValueTypedObject& other) const;
    bool operator!=(const ValueTypedObject& other) const;

    static Ref<ValueTypedObject> make(const Ref<ClassSchema>& classSchema);
    static Ref<ValueTypedObject> make(const Ref<ClassSchema>& classSchema, std::initializer_list<Value> properties) {
        auto typedObject = make(classSchema);

        size_t index = 0;
        for (auto& propertyValue : properties) {
            typedObject->setProperty(index, std::move(propertyValue));
            index++;
        }

        return typedObject;
    }

private:
    Ref<ClassSchema> _classSchema;

    friend InlineContainerAllocator<ValueTypedObject, Value>;

    ValueTypedObject(const Ref<ClassSchema>& classSchema);
};

} // namespace Valdi
