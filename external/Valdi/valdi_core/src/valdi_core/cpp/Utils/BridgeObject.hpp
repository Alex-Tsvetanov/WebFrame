//
//  BridgeObject.hpp
//  snapchat_desktop_core
//
//  Created by Simon Corsin on 10/7/20.
//

#pragma once

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

#include <unordered_map>

namespace Valdi {

class ILogger;

class BridgeContext : public SimpleRefCountable, public ExceptionTracker {
public:
    BridgeContext(ILogger& logger, const Ref<ValueFunction>& errorHandler);
    ~BridgeContext() override;

protected:
    Error onGetError() const final;
    void onClearError() final;
    void onSetError(Error&& error) final;

private:
    [[maybe_unused]] ILogger& _logger;
    Ref<ValueFunction> _errorHandler;
    std::optional<Error> _error;
};

class BridgeObject : public ValdiObject {
public:
    BridgeObject(const Value& object, const Ref<BridgeContext>& bridgeContext);
    ~BridgeObject() override;

    template<typename... T>
    void call(const StringBox& method, const T&... parameters) {
        std::initializer_list<Value> parametersList = {parameters...};

        doCall(method, parametersList.begin(), parametersList.size(), ValueFunctionFlagsNone);
    }

    template<typename... T>
    Value callSync(const StringBox& method, const T&... parameters) {
        std::initializer_list<Value> parametersList = {parameters...};

        return doCall(method, parametersList.begin(), parametersList.size(), ValueFunctionFlagsCallSync);
    }

    Value doCall(const StringBox& method, const Value* parameters, size_t parametersSize, ValueFunctionFlags flags);

    const Ref<BridgeContext>& getBridgeContext() const;

    VALDI_CLASS_HEADER(BridgeObject)

private:
    Value _object;
    Ref<BridgeContext> _bridgeContext;
};

class WrappedNativeInstance : public ValdiObject {
public:
    explicit WrappedNativeInstance(const std::shared_ptr<void>& ptr);
    ~WrappedNativeInstance() override;

    constexpr const std::shared_ptr<void>& ptr() const {
        return _ptr;
    }

    VALDI_CLASS_HEADER(WrappedNativeInstance)

private:
    std::shared_ptr<void> _ptr;
};

Value marshallString(const std::string& value, const Ref<BridgeContext>& bridgeContext);
Value marshallInt32(int32_t value, const Ref<BridgeContext>& bridgeContext);
Value marshallInt64(int64_t value, const Ref<BridgeContext>& bridgeContext);
Value marshallFloat32(float value, const Ref<BridgeContext>& bridgeContext);
Value marshallBool(bool value, const Ref<BridgeContext>& bridgeContext);
Value marshallBinary(const std::vector<uint8_t>& value, const Ref<BridgeContext>& bridgeContext);

template<typename T, typename Convert>
Value marshallOptional(const std::optional<T>& value, const Ref<BridgeContext>& /*bridgeContext*/, Convert&& convert) {
    if (!value) {
        return Value::undefined();
    }

    return convert(*value);
}

template<typename T, typename Convert>
Value marshallOptionalPtr(const T& value, const Ref<BridgeContext>& /*bridgeContext*/, Convert&& convert) {
    if (value == nullptr) {
        return Value::undefined();
    }

    return convert(value);
}

template<typename MapKey, typename MapValue, typename ConvertKey, typename ConvertValue>
Value marshallMap(const std::unordered_map<MapKey, MapValue>& value,
                  const Ref<BridgeContext>& /*bridgeContext*/,
                  ConvertKey&& convertKey,
                  ConvertValue&& convertValue) {
    auto map = makeShared<ValueMap>();

    for (const auto& it : value) {
        auto convertedKey = convertKey(it.first);
        auto convertedValue = convertValue(it.second);
        (*map)[convertedKey.toStringBox()] = convertedValue;
    }

    return Value(map);
}

template<typename VecValue, typename ConvertValue>
Value marshallVector(const std::vector<VecValue>& value,
                     const Ref<BridgeContext>& /*bridgeContext*/,
                     ConvertValue&& convertValue) {
    auto out = ValueArray::make(value.size());

    size_t index = 0;

    for (const auto& it : value) {
        out->emplace(index, convertValue(it));
        index++;
    }

    return Value(out);
}

template<typename T>
Value marshallBridgeObject(const std::shared_ptr<T>& bridgeObject, const Ref<BridgeContext>& /*bridgeContext*/) {
    Valdi::Value out;

    if (bridgeObject != nullptr) {
        auto valdiObject = std::dynamic_pointer_cast<ValdiObject>(bridgeObject);
        if (valdiObject == nullptr) {
            valdiObject = makeShared<WrappedNativeInstance>(bridgeObject).toShared();
        }

        out.setMapValue("$nativeInstance", Valdi::Value(valdiObject));
    }

    return out;
}

std::string unmarshallString(const Value& value, const Ref<BridgeContext>& bridgeContext);
int32_t unmarshallInt32(const Value& value, const Ref<BridgeContext>& bridgeContext);
int64_t unmarshallInt64(const Value& value, const Ref<BridgeContext>& bridgeContext);
float unmarshallFloat32(const Value& value, const Ref<BridgeContext>& bridgeContext);
bool unmarshallBool(const Value& value, const Ref<BridgeContext>& bridgeContext);
std::vector<uint8_t> unmarshallBinary(const Value& value, const Ref<BridgeContext>& bridgeContext);

template<typename T, typename Convert>
std::optional<T> unmarshallOptional(const Value& value,
                                    const Ref<BridgeContext>& /*bridgeContext*/,
                                    Convert&& convert) {
    if (value.isNullOrUndefined()) {
        return std::nullopt;
    }

    return {convert(value)};
}

template<typename T, typename Convert>
T unmarshallOptionalPtr(const Value& value, const Ref<BridgeContext>& /*bridgeContext*/, Convert&& convert) {
    if (value.isNullOrUndefined()) {
        return nullptr;
    }

    return convert(value);
}

template<typename MapKey, typename MapValue, typename ConvertKey, typename ConvertValue>
std::unordered_map<MapKey, MapValue> unmarshallMap(const Value& value,
                                                   const Ref<BridgeContext>& /*bridgeContext*/,
                                                   ConvertKey&& convertKey,
                                                   ConvertValue&& convertValue) {
    std::unordered_map<MapKey, MapValue> out;

    if (value.isMap()) {
        for (const auto& it : *value.getMap()) {
            out[convertKey(Value(it.first))] = convertValue(it.second);
        }
    }

    return out;
}

template<typename VecValue, typename ConvertValue>
std::vector<VecValue> unmarshallVector(const Value& value,
                                       const Ref<BridgeContext>& /*bridgeContext*/,
                                       ConvertValue&& convertValue) {
    std::vector<VecValue> out;

    if (value.isArray()) {
        const auto& array = *value.getArray();
        out.reserve(array.size());
        for (const auto& item : array) {
            out.emplace_back(convertValue(item));
        }
    }

    return out;
}

template<typename T>
std::shared_ptr<T> unmarshallBridgeObject(const Value& value, const Ref<BridgeContext>& /*bridgeContext*/) {
    auto object = value.getMapValue("$nativeInstance").getValdiObject();
    if (object == nullptr) {
        return nullptr;
    }

    auto nativeInstance = Valdi::castOrNull<WrappedNativeInstance>(object);
    if (nativeInstance != nullptr) {
        return std::static_pointer_cast<T>(nativeInstance->ptr());
    }

    return std::dynamic_pointer_cast<T>(*object->getInnerSharedPtr());
}

} // namespace Valdi
