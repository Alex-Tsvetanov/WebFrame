//
//  HexUtils.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/29/20.
//

#include "valdi/runtime/Utils/HexUtils.hpp"

#include <iomanip>
#include <sstream>

namespace Valdi {

size_t hexStringToBytes(const std::string_view& hexString, Byte* output, size_t outputLength) {
    size_t strIndex = 0;

    for (size_t i = 0; i < outputLength; i++) {
        if (strIndex + 2 > hexString.size()) {
            return 0;
        }

        std::string tok;
        tok.append(hexString.data() + strIndex, 2);
        uint8_t byte = strtol(tok.data(), nullptr, 16);

        output[i] = byte;

        strIndex += 2;
    }

    return strIndex;
}

std::string bytesToHexString(const Byte* input, size_t inputLength) {
    std::stringstream stream;
    stream << std::hex << std::setfill('0');

    for (size_t i = 0; i < inputLength; i++) {
        auto b = input[i];
        stream << std::setw(2) << static_cast<int>(b);
    }

    return stream.str();
}

} // namespace Valdi
