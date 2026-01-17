//
//  DataEncryptor.cpp
//  valdi
//
//  Created by Simon Corsin on 9/8/22.
//

#include "valdi/runtime/Utils/DataEncryptor.hpp"

#include <openssl/rand.h>
#include <openssl/span.h>

namespace Valdi {

DataEncryptor::DataEncryptor(const snap::utils::crypto::AesEncryptor::Key& key) : _key(key) {}

static bssl::Span<uint8_t> toSpan(const Byte* data, size_t length) {
    return bssl::Span<uint8_t>(const_cast<uint8_t*>(data), length);
}

bool DataEncryptor::encrypt(const Byte* data, size_t length, ByteBuffer& out) const {
    snap::utils::crypto::AesEncryptor::Iv iv;
    RAND_bytes(reinterpret_cast<uint8_t*>(iv.data()), iv.size());

    snap::utils::crypto::AesEncryptor encryptor(_key, iv);

    auto encryptResult = encryptor.encrypt(toSpan(data, length));
    if (!encryptResult) {
        return false;
    }

    // Insert the IV at the front
    out.append(iv.begin(), iv.end());

    // Append the data after
    out.append(encryptResult.value().data(), encryptResult.value().data() + encryptResult.value().size());

    return true;
}

bool DataEncryptor::decrypt(const Byte* data, size_t length, ByteBuffer& out) const {
    snap::utils::crypto::AesEncryptor::Iv iv;

    if (length < iv.size()) {
        return false;
    }

    std::memcpy(iv.data(), data, iv.size());

    snap::utils::crypto::AesEncryptor decrypter(_key, iv);

    auto decryptResult = decrypter.decrypt(toSpan(data + iv.size(), length - iv.size()));

    if (!decryptResult) {
        return false;
    }

    out.append(decryptResult.value().data(), decryptResult.value().data() + decryptResult.value().size());

    return true;
}

snap::utils::crypto::AesEncryptor::Key DataEncryptor::generateKey() {
    snap::utils::crypto::AesEncryptor::Key key;
    RAND_bytes(reinterpret_cast<uint8_t*>(key.data()), key.size());

    return key;
}

} // namespace Valdi
