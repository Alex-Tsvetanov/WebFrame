//
//  AndroidKeychain.hpp
//  valdi-android
//
//  Created by Simon Corsin on 9/7/22.
//

#pragma once

#include "valdi/Keychain.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

#include "utils/crypto/AesEncryptor.hpp"

namespace ValdiAndroid {

/**
 * The AndroidKeychain is a Keychain implementation that leverages a provided
 * Java instance under the hood. Most of the encryption/decryption operations happens
 * at the C++ level within the implementation of this class, through a private key
 * that is provided by the Java implementation. The actual key/value storage is
 * delegated to Java, which uses Android's SharedPreferences under the hood.
 *
 * Java is responsible for retrieving and/or generating a private key that gets returned to C++,
 * and for storing/erasing/fetching items.
 * C++ is responsible for encrypting/decrypting items before passing the data to Java, with the
 * private key that is returned by Java.
 */
class AndroidKeychain : public snap::valdi::Keychain {
public:
    explicit AndroidKeychain(jobject keychainJava);
    ~AndroidKeychain() override;

    Valdi::PlatformResult store(const Valdi::StringBox& key, const Valdi::BytesView& value) override;

    Valdi::BytesView get(const Valdi::StringBox& key) override;

    bool erase(const Valdi::StringBox& key) override;

private:
    GlobalRefJavaObjectBase _keychainJava;
    JavaMethod<ByteArray> _getSecretKeyMethod;
    JavaMethod<String, String> _getMethod;
    JavaMethod<String, String, String> _setMethod;
    JavaMethod<bool, String> _eraseMethod;
    Valdi::Mutex _mutex;
    std::optional<snap::utils::crypto::AesEncryptor::Key> _secretKey;
    bool _fetchedSecretKey = false;

    std::optional<snap::utils::crypto::AesEncryptor::Key> getSecretKey();
};

} // namespace ValdiAndroid
