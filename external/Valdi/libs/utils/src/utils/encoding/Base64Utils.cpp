#include "utils/encoding/Base64Utils.hpp"

#include <boost/algorithm/string.hpp>
#include <openssl/base64.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

// We wrap the BoringSSL Base64 encoding/decoding functions
namespace snap::utils::encoding {

std::string binaryToBase64(const uint8_t* data, size_t size) {
    size_t maxOutSize = 0;
    if (size == 0 || EVP_EncodedLength(&maxOutSize, size) != 1 || maxOutSize == 0) {
        return "";
    }

    std::string result(maxOutSize, '\0');
    EVP_EncodeBlock(reinterpret_cast<uint8_t*>(result.data()), data, size);
    result.resize(result.size() - 1); // remove the trailing \0 that EVP_EncodeBlock always writes
    return result;
}

std::string binaryToBase64(const std::vector<uint8_t>& binary) {
    return binaryToBase64(binary.data(), binary.size());
}

std::string binaryToBase64(const std::string& binary) {
    return binaryToBase64(reinterpret_cast<const uint8_t*>(binary.data()), binary.size());
}

std::string uint64ToBase64(uint64_t data) {
    std::array<uint8_t, sizeof(data)> bytes;
    for (size_t i = 0; i < 8; i++) {
        bytes.at(i) = (data >> (i * 8)) & 0xFF;
    }
    return binaryToBase64(bytes.data(), bytes.size());
}

bool base64ToBinaryInternal(const char* encodedString, size_t inSize, std::vector<uint8_t>* ret) {
    // Strip out newlines since the decoder returns an error code (0) if it finds them within the encoded string.
    std::string noNewlines(encodedString, inSize);
    boost::algorithm::replace_all(noNewlines, "\n", "");
    boost::algorithm::replace_all(noNewlines, "\r", "");

    // Determine the max array length we'll need given the string length
    size_t maxOutSize = 0;
    if (EVP_DecodedLength(&maxOutSize, noNewlines.length()) != 1 || maxOutSize == 0) {
        return false;
    }

    ret->resize(maxOutSize);
    size_t outSize = 0;
    if (EVP_DecodeBase64(reinterpret_cast<uint8_t*>(ret->data()),
                         &outSize,
                         maxOutSize,
                         reinterpret_cast<const uint8_t*>(noNewlines.c_str()),
                         noNewlines.length()) != 1) {
        // make it empty to indicate error
        ret->resize(0);
        return false;
    }

    ret->resize(outSize);
    return true;
}

std::vector<uint8_t> base64ToBinary(std::string_view base64) {
    std::vector<uint8_t> decodedData;
    base64ToBinaryInternal(base64.data(), base64.size(), &decodedData);
    return decodedData;
}

bool base64ToBinary(std::string_view base64, std::vector<uint8_t>& decodedData) {
    return base64ToBinaryInternal(base64.data(), base64.size(), &decodedData);
}

uint64_t base64ToUInt64(const std::string& base64) {
    std::vector<uint8_t> bytes = base64ToBinary(base64);
    uint64_t retVal = 0;

    for (int i = std::min(static_cast<int>(bytes.size()), 8) - 1; i >= 0; i--) {
        retVal <<= 8;
        retVal |= bytes[i];
    }

    return retVal;
}

std::string base64UrlToBase64(const std::string& base64url) {
    std::string temp;
    temp.reserve(base64url.size() + 4);

    // change Base64 alphabet from urlsafe version to standard
    for (const auto& c : base64url) {
        if (c == '-') {
            temp += '+';
        } else if (c == '_') {
            temp += '/';
        } else {
            temp += c;
        }
    }

    // add padding
    if ((base64url.size() % 4) != 0u) {
        int toAppend = 4 - static_cast<int>(base64url.size() % 4);
        for (int i = 0; i < toAppend; i++) {
            temp += '=';
        }
    }

    return temp;
}

std::string base64ToBase64Url(const std::string& base64) {
    std::string temp(base64);

    // remove padding
    size_t found = temp.find_last_not_of('=');
    if (found == std::string::npos)
        return "";

    // change Base64 alphabet from standard version to urlsafe
    boost::algorithm::replace_all(temp, "+", "-");
    boost::algorithm::replace_all(temp, "/", "_");

    return temp.substr(0, found + 1);
}

} // namespace snap::utils::encoding
