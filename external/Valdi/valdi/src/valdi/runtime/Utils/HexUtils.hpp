//
//  HexUtils.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/29/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"

#include <string>
#include <string_view>

namespace Valdi {

/**
 Deserialize an hex string into a bytes output.
 Returns how many bytes were read.
 Returns 0 in case of failure.
 */
size_t hexStringToBytes(const std::string_view& hexString, Byte* output, size_t outputLength);

/**
 Serialize a bytes into a hex string.
*/
std::string bytesToHexString(const Byte* input, size_t inputLength);

} // namespace Valdi
