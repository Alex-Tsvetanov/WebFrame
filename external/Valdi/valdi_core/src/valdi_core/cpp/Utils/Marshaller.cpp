//
//  Marshaller.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/19/19.
//

#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"

#include "valdi_core/cpp/Utils/Promise.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include "valdi_core/cpp/Utils/ValueMapIterator.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"
#include "valdi_core/cpp/Utils/ValueTypedObject.hpp"
#include "valdi_core/cpp/Utils/ValueTypedProxyObject.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace Valdi {

constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();

Marshaller::Marshaller(ExceptionTracker& exceptionTracker)
    : _exceptionTracker(exceptionTracker), _currentSchema(ValueSchema::untyped()) {}

Marshaller::~Marshaller() = default;

ExceptionTracker& Marshaller::getExceptionTracker() const {
    return _exceptionTracker;
}

int Marshaller::push(Value value) {
    auto size = _stack.size();
    _stack.emplace_back(std::move(value));
    return static_cast<int>(size);
}

int Marshaller::pushString(const StringBox& str) {
    return push(Value(str));
}

int Marshaller::pushTypedArray(const Ref<ValueTypedArray>& typedArray) {
    return push(Value(typedArray));
}

int Marshaller::pushBool(bool boolean) {
    return push(Value(boolean));
}

int Marshaller::pushInt(int32_t value) {
    return push(Value(value));
}

int Marshaller::pushLong(int64_t value) {
    return push(Value(value));
}

int Marshaller::pushDouble(double d) {
    return push(Value(d));
}

int Marshaller::pushFunction(const Ref<ValueFunction>& function) {
    return push(Value(function));
}

int Marshaller::pushValdiObject(const Ref<ValdiObject>& valdiObject) {
    return push(Value(valdiObject));
}

int Marshaller::pushUndefined() {
    return push(Value::undefined());
}

int Marshaller::pushNull() {
    return push(Value());
}

void Marshaller::popCount(int count) {
    while (count > 0) {
        pop();
        count--;
    }
}

void Marshaller::clear() {
    popCount(size());
    _exceptionTracker.clearError();
    _currentSchema = ValueSchema::untyped();
}

int Marshaller::mergeMaps(int leftIndex, int rightIndex) {
    auto newMapIndex = pushMap(0);

    copyMap(leftIndex, newMapIndex);
    if (!_exceptionTracker) {
        return 0;
    }

    copyMap(rightIndex, newMapIndex);
    if (!_exceptionTracker) {
        return 0;
    }

    return newMapIndex;
}

void Marshaller::copyMap(int mapIndex, int intoMapIndex) {
    auto iterator = pushMapIterator(mapIndex);
    if (!_exceptionTracker) {
        return;
    }

    while (pushMapIteratorNextEntry(iterator)) {
        auto key = getString(-2);
        if (!_exceptionTracker) {
            return;
        }

        putMapProperty(key, intoMapIndex);
        if (!_exceptionTracker) {
            return;
        }

        pop();
        if (!_exceptionTracker) {
            return;
        }
    }

    if (!_exceptionTracker) {
        return;
    }

    pop();
}

Ref<ValueArray> Marshaller::toArray() {
    auto n = size();

    auto out = ValueArray::make(static_cast<size_t>(n));

    for (int i = 0; i < n; i++) {
        auto value = get(i);
        if (!_exceptionTracker) {
            return nullptr;
        }

        out->emplace(static_cast<size_t>(i), std::move(value));
    }

    return out;
}

std::string Marshaller::toString(bool indent) {
    auto array = toArray();
    return Value(array).toString(indent);
}

bool Marshaller::getBool(int index) {
    return getOrUndefinedAsRef(index).checkedTo<bool>(_exceptionTracker);
}

int Marshaller::getInt(int index) {
    return getOrUndefinedAsRef(index).checkedTo<int32_t>(_exceptionTracker);
}

int64_t Marshaller::getLong(int index) {
    return getOrUndefinedAsRef(index).checkedTo<int64_t>(_exceptionTracker);
}

double Marshaller::getDouble(int index) {
    return getOrUndefinedAsRef(index).checkedTo<double>(_exceptionTracker);
}

Ref<ValueTypedArray> Marshaller::getTypedArray(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValueTypedArray>>(_exceptionTracker);
}

Ref<ValueFunction> Marshaller::getFunction(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValueFunction>>(_exceptionTracker);
}

Ref<ValueMap> Marshaller::getMap(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValueMap>>(_exceptionTracker);
}

Ref<ValueTypedObject> Marshaller::getTypedObject(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValueTypedObject>>(_exceptionTracker);
}

Ref<ValueTypedProxyObject> Marshaller::getProxyObject(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValueTypedProxyObject>>(_exceptionTracker);
}

Ref<ValueArray> Marshaller::getArray(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValueArray>>(_exceptionTracker);
}

Ref<ValdiObject> Marshaller::getValdiObject(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<ValdiObject>>(_exceptionTracker);
}

const Value& Marshaller::getOrUndefinedAsRef(int index) {
    auto resolvedIndex = convertIndex(index);
    if (resolvedIndex == kInvalidIndex) {
        return Value::undefinedRef();
    }

    return _stack[resolvedIndex];
}

Value Marshaller::getOrUndefined(int index) {
    return getOrUndefinedAsRef(index);
}

void Marshaller::onPromiseComplete(int index, const Ref<ValueFunction>& callback) {
    auto promise = getOrUndefinedAsRef(index).checkedToValdiObject<Promise>(_exceptionTracker);
    if (promise != nullptr) {
        promise->onComplete([callback](const Result<Value>& result) {
            if (result) {
                (*callback)({result.value()});
            } else {
                (*callback)({Value(result.error())});
            }
        });
    }
}

bool Marshaller::operator!=(const Marshaller& other) const {
    return !(*this == other);
}

int Marshaller::size() {
    return static_cast<int>(_stack.size());
}

const Value& Marshaller::getAsRef(int index) {
    auto resolvedIndex = convertIndexOrError(index);
    if (!_exceptionTracker) {
        return Value::undefinedRef();
    }

    return _stack[resolvedIndex];
}

Value Marshaller::get(int index) {
    return getAsRef(index);
}

StringBox Marshaller::getString(int index) {
    return getOrUndefinedAsRef(index).checkedTo<StringBox>(_exceptionTracker);
}

Ref<StaticString> Marshaller::getStaticString(int index) {
    return getOrUndefinedAsRef(index).checkedTo<Ref<StaticString>>(_exceptionTracker);
}

void Marshaller::pop() {
    if (_stack.empty()) {
        _exceptionTracker.onError(Error("Stack is empty"));
        return;
    }
    _stack.pop_back();
}

void Marshaller::swap(int leftIndex, int rightIndex) {
    auto resolvedLeftIndex = convertIndexOrError(leftIndex);
    if (!_exceptionTracker) {
        return;
    }
    auto resolvedRightIndex = convertIndexOrError(rightIndex);
    if (!_exceptionTracker) {
        return;
    }

    std::swap(_stack[resolvedLeftIndex], _stack[resolvedRightIndex]);
}

ValueType Marshaller::getType(int index) {
    return getOrUndefinedAsRef(index).getType();
}

bool Marshaller::isNullOrUndefined(int index) {
    return getOrUndefinedAsRef(index).isNullOrUndefined();
}

bool Marshaller::isError(int index) {
    return getOrUndefinedAsRef(index).isError();
}

bool Marshaller::isString(int index) {
    return getOrUndefinedAsRef(index).isString();
}

bool Marshaller::isStaticString(int index) {
    return getOrUndefinedAsRef(index).isStaticString();
}

bool Marshaller::isTypedObject(int index) {
    return getOrUndefinedAsRef(index).isTypedObject();
}

bool Marshaller::isProxyObject(int index) {
    return getOrUndefinedAsRef(index).isProxyObject();
}

int Marshaller::pushArray(int expectedCapacity) {
    auto array = ValueArray::make(static_cast<size_t>(expectedCapacity));

    return push(Value(array));
}

int Marshaller::getArrayLength(int index) {
    auto array = getArray(index);
    if (!_exceptionTracker) {
        return 0;
    }
    return static_cast<int>(array->size());
}

int Marshaller::getArrayItem(int index, int arrayIndex) {
    auto array = getArray(index);
    if (!_exceptionTracker) {
        return 0;
    }

    auto arrayIndexSizeT = static_cast<size_t>(arrayIndex);
    if (arrayIndex < 0 || arrayIndexSizeT >= array->size()) {
        _exceptionTracker.onError(
            Error(STRING_FORMAT("Out of bounds index {}, array size is {}", arrayIndex, array->size())));
        return 0;
    }
    return push((*array)[arrayIndexSizeT]);
}

void Marshaller::setArrayItem(int index, int arrayIndex) {
    auto array = getArray(index);
    if (!_exceptionTracker) {
        return;
    }

    auto valueToPlace = get(-1);
    if (!_exceptionTracker) {
        return;
    }
    pop();

    auto arrayIndexSizeT = static_cast<size_t>(arrayIndex);
    if (arrayIndex < 0 || arrayIndexSizeT >= array->size()) {
        _exceptionTracker.onError(
            Error(STRING_FORMAT("Out of bounds index {}, array size is {}", arrayIndex, array->size())));
        return;
    }

    array->emplace(static_cast<int>(arrayIndex), std::move(valueToPlace));
}

int Marshaller::pushMap(int expectedCapacity) {
    auto map = makeShared<ValueMap>();
    map->reserve(static_cast<size_t>(expectedCapacity));

    return push(Value(map));
}

int Marshaller::getMapLength(int index) {
    auto map = getMap(index);
    if (!_exceptionTracker) {
        return 0;
    }

    return static_cast<int>(map->size());
}

bool Marshaller::getMapProperty(const StringBox& key, int index, Marshaller::GetMapPropertyFlags flags) {
    auto map = getMap(index);
    if (!_exceptionTracker) {
        return false;
    }
    const auto& it = map->find(key);
    if (it == map->end()) {
        if (flags == GetMapPropertyFlags::PushUndefinedOnMiss) {
            pushUndefined();
            return true;
        }

        return false;
    }

    if (flags == GetMapPropertyFlags::IgnoreNullOrUndefined && it->second.isNullOrUndefined()) {
        return false;
    }

    push(it->second);
    return true;
}

void Marshaller::mustGetMapProperty(const StringBox& key, int index, Marshaller::GetMapPropertyFlags flags) {
    if (!getMapProperty(key, index, flags)) {
        if (_exceptionTracker) {
            _exceptionTracker.onError(Error(STRING_FORMAT("Object has no property '{}'", key)));
        }
    }
}

void Marshaller::putMapProperty(int index) {
    // Only string keys are supported in ValueMap at the moment
    auto key = getString(-2);
    if (!_exceptionTracker) {
        return;
    }

    putMapProperty(key, index);
    pop();
}

void Marshaller::putMapProperty(const StringBox& key, int index) {
    auto map = getMap(index);
    if (!_exceptionTracker) {
        return;
    }

    auto valueToPlace = get(-1);
    if (!_exceptionTracker) {
        return;
    }
    pop();
    (*map)[key] = valueToPlace;
}

int Marshaller::pushMapIterator(int index) {
    auto map = getMap(index);
    if (!_exceptionTracker) {
        return 0;
    }

    return push(Value(makeShared<ValueMapIterator>(map)));
}

bool Marshaller::pushMapIteratorNextEntry(int index) {
    auto iterator = getOrUndefinedAsRef(index).checkedToValdiObject<ValueMapIterator>(_exceptionTracker);
    if (!_exceptionTracker) {
        return false;
    }

    auto entry = iterator->next();
    if (entry) {
        push(Value(entry->first));
        push(std::move(entry->second));
        return true;
    } else {
        return false;
    }
}

int Marshaller::duplicate(int index) {
    auto value = get(index);
    if (!_exceptionTracker) {
        return 0;
    }
    return push(std::move(value));
}

bool Marshaller::operator==(const Marshaller& other) const {
    if (this == &other) {
        return true;
    }

    return _stack == other._stack;
}

void Marshaller::setCurrentSchema(const ValueSchema& currentSchema) {
    _currentSchema = currentSchema;
}

const ValueSchema& Marshaller::getCurrentSchema() const {
    return _currentSchema;
}

size_t Marshaller::convertIndex(int index) {
    if (index >= 0) {
        auto arrayIndex = static_cast<size_t>(index);
        if (arrayIndex >= _stack.size()) {
            return kInvalidIndex;
        }
        return arrayIndex;
    } else {
        auto offsetFromTop = static_cast<size_t>(-index);
        if (offsetFromTop > _stack.size()) {
            return kInvalidIndex;
        }
        return static_cast<size_t>(_stack.size() - offsetFromTop);
    }
}

size_t Marshaller::convertIndexOrError(int index) {
    auto resolvedIndex = convertIndex(index);

    if (resolvedIndex == kInvalidIndex) {
        _exceptionTracker.onError(
            Error(STRING_FORMAT("Out of bounds index {} in Marshaller, has {} values", index, size())));
    }

    return resolvedIndex;
}

const Value* Marshaller::getValues() const {
    return _stack.data();
}

} // namespace Valdi
