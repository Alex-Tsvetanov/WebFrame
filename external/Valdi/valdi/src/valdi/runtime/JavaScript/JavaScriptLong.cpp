//
//  JavaScriptLong.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 2/5/20.
//

#include "valdi/runtime/JavaScript/JavaScriptLong.hpp"

namespace Valdi {

static uint64_t packBits(int32_t lowBits, int32_t highBits) {
    return static_cast<uint64_t>(
        ((static_cast<int64_t>(lowBits) & 0xFFFFFFFF) | (static_cast<int64_t>(highBits) << 32)));
}

JavaScriptLong::JavaScriptLong() = default;
JavaScriptLong::JavaScriptLong(uint64_t unsignedInt) : _data(unsignedInt), _isUnsigned(true) {}
JavaScriptLong::JavaScriptLong(int64_t signedInt) : _data(static_cast<uint64_t>(signedInt)) {}
JavaScriptLong::JavaScriptLong(int32_t lowBits, int32_t highBits, bool isUnsigned)
    : _data(packBits(lowBits, highBits)), _isUnsigned(isUnsigned) {}

uint64_t JavaScriptLong::toUInt64() const {
    return _data;
}

int64_t JavaScriptLong::toInt64() const {
    return static_cast<int64_t>(_data);
}

int32_t JavaScriptLong::toInt32() const {
    return static_cast<int32_t>(_data);
}

int32_t JavaScriptLong::highBits() const {
    return static_cast<int32_t>(_data >> 32 & 0xFFFFFFFF);
}

int32_t JavaScriptLong::lowBits() const {
    return static_cast<int32_t>(_data & 0xFFFFFFFF);
}

bool JavaScriptLong::isUnsigned() const {
    return _isUnsigned;
}

} // namespace Valdi
