//
//  DataEncryptor.hpp
//  valdi
//
//  Created by Simon Corsin on 9/8/22.
//

#pragma once

#include "utils/crypto/AesEncryptor.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {

/**
 * The DataEncryptor is a class that simplifies encrypting and decrypting data
 * using a given private and securely generated AES Key.
 * The data is encoded with a secure random IV that will be stored
 * to the given ByteBuffer.
 */
class DataEncryptor {
public:
    explicit DataEncryptor(const snap::utils::crypto::AesEncryptor::Key& key);

    bool encrypt(const Byte* data, size_t length, ByteBuffer& out) const;
    bool decrypt(const Byte* data, size_t length, ByteBuffer& out) const;

    static snap::utils::crypto::AesEncryptor::Key generateKey();

private:
    snap::utils::crypto::AesEncryptor::Key _key;
};

} // namespace Valdi
