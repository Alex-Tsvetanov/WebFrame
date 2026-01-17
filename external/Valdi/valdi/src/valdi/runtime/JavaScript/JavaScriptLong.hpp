//
//  JavaScriptLong.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 2/5/20.
//

#pragma once

#include <cstdint>

namespace Valdi {

class JavaScriptLong {
public:
    JavaScriptLong();

    explicit JavaScriptLong(uint64_t unsignedInt);
    explicit JavaScriptLong(int64_t signedInt);
    JavaScriptLong(int32_t lowBits, int32_t highBits, bool isUnsigned);

    uint64_t toUInt64() const;

    int64_t toInt64() const;

    int32_t toInt32() const;

    int32_t highBits() const;

    int32_t lowBits() const;

    bool isUnsigned() const;

private:
    uint64_t _data{0};
    bool _isUnsigned{false};
};

} // namespace Valdi
