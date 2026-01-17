//
//  Marshaller.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/19/19.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Utils/ExceptionTracker.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

#include <optional>
#include <string>

namespace Valdi {

constexpr size_t kMarshallerSmallSize = 4;
class ClassSchema;
class ValueTypedObject;
class ExceptionTracker;

class Marshaller : public snap::NonCopyable {
public:
    enum class GetMapPropertyFlags {
        None,
        IgnoreNullOrUndefined,
        PushUndefinedOnMiss,
    };

    Marshaller(ExceptionTracker& exceptionTracker);
    virtual ~Marshaller();

    int size();

    ExceptionTracker& getExceptionTracker() const;

    int push(Value value);
    int pushString(const StringBox& str);
    int pushTypedArray(const Ref<ValueTypedArray>& typedArray);
    int pushBool(bool boolean);
    int pushInt(int32_t value);
    int pushLong(int64_t value);
    int pushDouble(double d);
    int pushFunction(const Ref<ValueFunction>& function);
    int pushUndefined();
    int pushNull();
    int pushValdiObject(const Ref<ValdiObject>& valdiObject);

    Value get(int index);
    StringBox getString(int index);
    Ref<StaticString> getStaticString(int index);
    bool getBool(int index);
    int getInt(int index);
    int64_t getLong(int index);
    double getDouble(int index);
    Ref<ValueTypedArray> getTypedArray(int index);
    Ref<ValueFunction> getFunction(int index);
    Ref<ValueMap> getMap(int index);
    Ref<ValueArray> getArray(int index);
    Ref<ValdiObject> getValdiObject(int index);
    Ref<ValueTypedObject> getTypedObject(int index);
    Ref<ValueTypedProxyObject> getProxyObject(int index);
    Value getOrUndefined(int index);

    void onPromiseComplete(int index, const Ref<ValueFunction>& callback);

    void pop();
    void popCount(int count);
    void clear();

    void swap(int leftIndex, int rightIndex);

    ValueType getType(int index);
    bool isNullOrUndefined(int index);
    bool isError(int index);
    bool isStaticString(int index);
    bool isString(int index);
    bool isTypedObject(int index);
    bool isProxyObject(int index);

    int pushArray(int expectedCapacity);
    int getArrayLength(int index);
    int getArrayItem(int index, int arrayIndex);
    void setArrayItem(int index, int arrayIndex);

    int pushMap(int expectedCapacity);
    int getMapLength(int index);
    bool getMapProperty(const StringBox& key, int index, GetMapPropertyFlags flags = GetMapPropertyFlags::None);
    void mustGetMapProperty(const StringBox& key, int index, GetMapPropertyFlags flags = GetMapPropertyFlags::None);
    void putMapProperty(const StringBox& key, int index);
    void putMapProperty(int index);
    int pushMapIterator(int index);
    bool pushMapIteratorNextEntry(int index);

    int mergeMaps(int leftIndex, int rightIndex);
    void copyMap(int mapIndex, int intoMapIndex);

    void setCurrentSchema(const ValueSchema& schema);
    const ValueSchema& getCurrentSchema() const;

    /**
     Create a shallow copy of the entry at the given index and move it
     at the top of the stack.
     */
    int duplicate(int index);

    /**
     * Convert the Marshaller backing stack into an array.
     */
    Ref<ValueArray> toArray();

    /**
     * Returns a string representation of the Marshaller.
     * Useful for debugging.
     */
    std::string toString(bool indent);

    const Value* getValues() const;

    bool operator==(const Marshaller& other) const;
    bool operator!=(const Marshaller& other) const;

private:
    ExceptionTracker& _exceptionTracker;
    SmallVector<Value, kMarshallerSmallSize> _stack;
    ValueSchema _currentSchema;

    size_t convertIndex(int index);
    size_t convertIndexOrError(int index);

    const Value& getAsRef(int index);
    const Value& getOrUndefinedAsRef(int index);
};

} // namespace Valdi
