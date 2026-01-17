//
//  AndroidKeychain.cpp
//  valdi-android
//
//  Created by Simon Corsin on 9/7/22.
//

#include "valdi/android/AndroidKeychain.hpp"
#include "utils/encoding/Base64Utils.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

#include "utils/debugging/Assert.hpp"
#include "valdi/runtime/Utils/DataEncryptor.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"

namespace ValdiAndroid {

AndroidKeychain::AndroidKeychain(jobject keychainJava) : _keychainJava(JavaEnv(), keychainJava, "Keychain") {
    auto keychainJavaObj = _keychainJava.toObject();
    auto cls = keychainJavaObj.getClass();
    cls.getMethod("getSecretKey", _getSecretKeyMethod);
    cls.getMethod("get", _getMethod);
    cls.getMethod("set", _setMethod);
    cls.getMethod("erase", _eraseMethod);
}

AndroidKeychain::~AndroidKeychain() = default;

Valdi::PlatformResult AndroidKeychain::store(const Valdi::StringBox& key, const Valdi::BytesView& value) {
    auto keyJava = toJavaObject(JavaEnv(), key);
    auto secretKey = getSecretKey();

    std::string base64Data;

    if (secretKey) {
        Valdi::DataEncryptor encryptor(*secretKey);

        Valdi::ByteBuffer buffer;
        if (!encryptor.encrypt(value.data(), value.size(), buffer)) {
            return Valdi::Error("Failed to encrypt data");
        }

        base64Data = snap::utils::encoding::binaryToBase64(buffer.data(), buffer.size());
    } else {
        base64Data = snap::utils::encoding::binaryToBase64(value.data(), value.size());
    }

    auto base64DataJava = toJavaObject(JavaEnv(), base64Data);

    auto errorMessage = _setMethod.call(_keychainJava.toObject(), keyJava, base64DataJava);

    if (!errorMessage.isNull()) {
        return Valdi::Error(toInternedString(JavaEnv(), errorMessage));
    }

    return Valdi::Value();
}

Valdi::BytesView AndroidKeychain::get(const Valdi::StringBox& key) {
    auto keyJava = toJavaObject(JavaEnv(), key);
    auto dataJava = _getMethod.call(_keychainJava.toObject(), keyJava);

    if (dataJava.isNull()) {
        return Valdi::BytesView();
    }

    auto dataBase64 = toInternedString(ValdiAndroid::JavaEnv(), dataJava);
    auto data = snap::utils::encoding::base64ToBinary(dataBase64.toStringView());
    auto output = Valdi::makeShared<Valdi::ByteBuffer>();

    auto secretKey = getSecretKey();

    if (secretKey) {
        Valdi::DataEncryptor encryptor(*secretKey);

        if (!encryptor.decrypt(data.data(), data.size(), *output)) {
            return Valdi::BytesView();
        }
    } else {
        output->append(data.data(), data.data() + data.size());
    }

    return output->toBytesView();
}

bool AndroidKeychain::erase(const Valdi::StringBox& key) {
    auto keyJava = toJavaObject(JavaEnv(), key);
    return _eraseMethod.call(_keychainJava.toObject(), keyJava);
}

std::optional<snap::utils::crypto::AesEncryptor::Key> AndroidKeychain::getSecretKey() {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    if (!_fetchedSecretKey) {
        _fetchedSecretKey = true;

        VALDI_TRACE("Valdi.getAndroidKeychainSecretKey")

        auto secretKeyJava = _getSecretKeyMethod.call(_keychainJava.toObject());
        if (!secretKeyJava.isNull()) {
            auto bytes =
                ValdiAndroid::toByteArray(JavaEnv(), reinterpret_cast<jbyteArray>(secretKeyJava.getUnsafeObject()));
            snap::utils::crypto::AesEncryptor::Key key;
            SC_ASSERT(sizeof(key) == bytes.size());

            std::memcpy(key.data(), bytes.data(), bytes.size());
            _secretKey = {key};
        }
    }

    return _secretKey;
}

} // namespace ValdiAndroid
