//
//  BytesUtils.cpp
//  valdi-ios
//
//  Created by saniul on 11/5/19.
//

#include "valdi/runtime/Utils/BytesUtils.hpp"
#include "valdi/runtime/Utils/HexUtils.hpp"

#include <array>
#include <openssl/sha.h>

namespace Valdi {

Ref<ByteBuffer> BytesUtils::sha256(const BytesView& data) {
    return sha256(data.data(), data.size());
}

static void doDigest(const Byte* data, size_t length, Byte* output) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, length);
    SHA256_Final(output, &sha256);
}

Ref<ByteBuffer> BytesUtils::sha256(const Byte* data, size_t length) {
    auto bytes = makeShared<ByteBuffer>();
    bytes->resize(SHA256_DIGEST_LENGTH);
    doDigest(data, length, bytes->data());

    return bytes;
}

std::string BytesUtils::sha256String(const BytesView& data) {
    return sha256String(data.data(), data.size());
}

std::string BytesUtils::sha256String(const Byte* data, size_t length) {
    std::array<Byte, SHA256_DIGEST_LENGTH> digest;
    doDigest(data, length, digest.data());

    return bytesToHexString(digest.data(), digest.size());
}

} // namespace Valdi
