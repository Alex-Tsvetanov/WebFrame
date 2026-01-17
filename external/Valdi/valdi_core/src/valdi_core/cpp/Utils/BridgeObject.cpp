//
//  BridgeObject.cpp
//  snapchat_desktop_core
//
//  Created by Simon Corsin on 10/7/20.
//

#include "valdi_core/cpp/Utils/BridgeObject.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace Valdi {

BridgeContext::BridgeContext(ILogger& logger, const Ref<ValueFunction>& errorHandler)
    : _logger(logger), _errorHandler(errorHandler) {}

BridgeContext::~BridgeContext() = default;

Error BridgeContext::onGetError() const {
    return _error.value();
}

void BridgeContext::onClearError() {
    _error = std::nullopt;
}

void BridgeContext::onSetError(Error&& error) {
    _error = std::make_optional<Error>(std::move(error));

    VALDI_ERROR(_logger, "Bridge error: {}", _error.value());
    if (_errorHandler != nullptr) {
        (*_errorHandler)({Value(_error.value().toString())});
    }
}

BridgeObject::BridgeObject(const Value& object, const Ref<BridgeContext>& bridgeContext)
    : _object(object), _bridgeContext(bridgeContext) {
    SC_ASSERT_NOTNULL(bridgeContext);
}

BridgeObject::~BridgeObject() = default;

VALDI_CLASS_IMPL(BridgeObject)

Value BridgeObject::doCall(const StringBox& method,
                           const Value* parameters,
                           size_t parametersSize,
                           ValueFunctionFlags flags) {
    auto function = _object.getMapValue(method);
    if (!function.isFunction()) {
        _bridgeContext->onError(Error(STRING_FORMAT("Cannot call '{}' since its not a function", method)));
        return Value::undefined();
    }

    return (*function.getFunction())(ValueFunctionCallContext(flags, parameters, parametersSize, *_bridgeContext));
}

const Ref<BridgeContext>& BridgeObject::getBridgeContext() const {
    return _bridgeContext;
}

WrappedNativeInstance::WrappedNativeInstance(const std::shared_ptr<void>& ptr) : _ptr(ptr) {}

WrappedNativeInstance::~WrappedNativeInstance() = default;

VALDI_CLASS_IMPL(WrappedNativeInstance)

Value marshallString(const std::string& value, const Ref<BridgeContext>& /*bridgeContext*/) {
    return Value(StringCache::getGlobal().makeString(value));
}

Value marshallInt32(int32_t value, const Ref<BridgeContext>& /*bridgeContext*/) {
    return Value(value);
}

Value marshallInt64(int64_t value, const Ref<BridgeContext>& /*bridgeContext*/) {
    return Value(value);
}

Value marshallFloat32(float value, const Ref<BridgeContext>& /*bridgeContext*/) {
    return Value(static_cast<double>(value));
}

Value marshallBool(bool value, const Ref<BridgeContext>& /*bridgeContext*/) {
    return Value(value);
}

Value marshallBinary(const std::vector<uint8_t>& value, const Ref<BridgeContext>& /*bridgeContext*/) {
    auto bytes = makeShared<ByteBuffer>();
    bytes->set(value.data(), value.data() + value.size());

    return Value(makeShared<ValueTypedArray>(TypedArrayType::Uint8Array, bytes->toBytesView()));
}

std::string unmarshallString(const Value& value, const Ref<BridgeContext>& bridgeContext) {
    return value.checkedTo<StringBox>(*bridgeContext).slowToString();
}

int32_t unmarshallInt32(const Value& value, const Ref<BridgeContext>& bridgeContext) {
    return value.checkedTo<int32_t>(*bridgeContext);
}

int64_t unmarshallInt64(const Value& value, const Ref<BridgeContext>& bridgeContext) {
    return value.checkedTo<int64_t>(*bridgeContext);
}

float unmarshallFloat32(const Value& value, const Ref<BridgeContext>& bridgeContext) {
    return static_cast<float>(value.checkedTo<double>(*bridgeContext));
}

bool unmarshallBool(const Value& value, const Ref<BridgeContext>& bridgeContext) {
    return value.checkedTo<bool>(*bridgeContext);
    ;
}

std::vector<uint8_t> unmarshallBinary(const Value& value, const Ref<BridgeContext>& bridgeContext) {
    auto typedArray = value.checkedTo<Ref<ValueTypedArray>>(*bridgeContext);
    if (typedArray == nullptr) {
        return {};
    }

    const auto& buffer = typedArray->getBuffer();

    std::vector<uint8_t> out;
    out.insert(out.begin(), buffer.begin(), buffer.end());
    return out;
}

} // namespace Valdi
