#include "valdi/runtime/Utils/DataEncryptor.hpp"
#include "gtest/gtest.h"

using namespace Valdi;

namespace ValdiTest {

TEST(DataEncryptor, canEncryptAndDecrypt) {
    auto key = DataEncryptor::generateKey();
    std::string_view dataToEncrypt = "Hello World!\n";

    DataEncryptor encryptor(key);

    ByteBuffer encryptedBuffer;

    ASSERT_TRUE(encryptor.encrypt(
        reinterpret_cast<const Byte*>(dataToEncrypt.data()), dataToEncrypt.length(), encryptedBuffer));

    ASSERT_TRUE(encryptedBuffer.size() > dataToEncrypt.length());

    ByteBuffer decryptedBuffer;

    ASSERT_TRUE(encryptor.decrypt(encryptedBuffer.data(), encryptedBuffer.size(), decryptedBuffer));

    std::string_view decryptedData(reinterpret_cast<const char*>(decryptedBuffer.data()), decryptedBuffer.size());

    ASSERT_EQ(dataToEncrypt, decryptedData);
}

TEST(DataEncryptor, failsToDecryptWithInvalidIV) {
    auto key = DataEncryptor::generateKey();

    std::string_view dataToEncrypt = "Hello World!\n";

    DataEncryptor encryptor(key);

    ByteBuffer encryptedBuffer;

    ASSERT_TRUE(encryptor.encrypt(
        reinterpret_cast<const Byte*>(dataToEncrypt.data()), dataToEncrypt.length(), encryptedBuffer));

    ByteBuffer invalidBuffer;
    invalidBuffer.resize(11);

    ByteBuffer decryptedBuffer;

    ASSERT_FALSE(encryptor.decrypt(invalidBuffer.data(), invalidBuffer.size(), decryptedBuffer));

    // Corrupt the IV
    (*encryptedBuffer[0])++;

    if (encryptor.decrypt(invalidBuffer.data(), invalidBuffer.size(), decryptedBuffer)) {
        // If we succeeded to decrypt with the altered IV, we should at least not have the expected output
        std::string_view decryptedData(reinterpret_cast<const char*>(decryptedBuffer.data()), decryptedBuffer.size());

        ASSERT_NE(dataToEncrypt, decryptedData);
    }
}

} // namespace ValdiTest
