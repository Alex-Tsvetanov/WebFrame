//
//  EncryptedDiskCache.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 1/30/24.
//

#include "valdi/runtime/Resources/EncryptedDiskCache.hpp"

#include "valdi_core/cpp/Utils/StringCache.hpp"

#include "valdi/Keychain.hpp"

namespace Valdi {

static constexpr const char* kKeychainKey = "EncryptedDiskCacheKey";

EncryptedDiskCache::EncryptedDiskCache(const Ref<IDiskCache>& diskCache, const Shared<snap::valdi::Keychain>& keychain)
    : EncryptedDiskCache(diskCache, keychain, std::nullopt) {}

EncryptedDiskCache::EncryptedDiskCache(const Ref<IDiskCache>& diskCache,
                                       const Shared<snap::valdi::Keychain>& keychain,
                                       const std::optional<snap::utils::crypto::AesEncryptor::Key>& cryptoKey)
    : _diskCache(diskCache), _keychain(keychain), _cryptoKey(cryptoKey) {}

EncryptedDiskCache::~EncryptedDiskCache() = default;

bool EncryptedDiskCache::exists(const Path& path) {
    return _diskCache->exists(path);
}

Result<BytesView> EncryptedDiskCache::load(const Path& path) {
    auto data = _diskCache->load(path);
    if (!data) {
        return data.moveError();
    }

    return decrypt(data.value());
}

Result<BytesView> EncryptedDiskCache::loadForAbsoluteURL(const StringBox& url) {
    auto data = _diskCache->loadForAbsoluteURL(url);
    if (!data) {
        return data.moveError();
    }

    return decrypt(data.value());
}

Result<Void> EncryptedDiskCache::store(const Path& path, const BytesView& bytes) {
    auto encrypted = encrypt(bytes);
    if (!encrypted) {
        return encrypted.moveError();
    }
    return _diskCache->store(path, encrypted.value());
}

bool EncryptedDiskCache::remove(const Path& path) {
    return _diskCache->remove(path);
}

StringBox EncryptedDiskCache::getAbsoluteURL(const Path& path) const {
    return _diskCache->getAbsoluteURL(path);
}

Ref<IDiskCache> EncryptedDiskCache::scopedCache(const Path& path, bool allowsReadOutsideOfScope) const {
    auto newDiskCache = _diskCache->scopedCache(path, allowsReadOutsideOfScope);
    return makeShared<EncryptedDiskCache>(newDiskCache, _keychain);
}

Path EncryptedDiskCache::getRootPath() const {
    return _diskCache->getRootPath();
}

std::vector<Path> EncryptedDiskCache::list(const Path& path) const {
    return _diskCache->list(path);
}

Result<BytesView> EncryptedDiskCache::encrypt(const BytesView& input) {
    auto out = makeShared<ByteBuffer>();

    if (!getDataEncryptor().encrypt(input.data(), input.size(), *out)) {
        return Error("Failed to encrypt data");
    }

    return out->toBytesView();
}

Result<BytesView> EncryptedDiskCache::decrypt(const BytesView& input) {
    auto out = makeShared<ByteBuffer>();
    if (!getDataEncryptor().decrypt(input.data(), input.size(), *out)) {
        return Error("Failed to derypt data");
    }

    return out->toBytesView();
}

StringBox EncryptedDiskCache::getKeychainKey() {
    return STRING_LITERAL(kKeychainKey);
}

void EncryptedDiskCache::generateOrRestoreCryptoKey() {
    getDataEncryptor();
}

DataEncryptor EncryptedDiskCache::getDataEncryptor() {
    std::lock_guard<Mutex> guard(_mutex);
    if (!_cryptoKey) {
        auto key = EncryptedDiskCache::getKeychainKey();
        if (!restoreCryptoKey(key)) {
            generateCryptoKey(key);
        }
    }

    return DataEncryptor(_cryptoKey.value());
}

bool EncryptedDiskCache::restoreCryptoKey(const StringBox& path) {
    auto encryptoKeyBinary = _keychain->get(path);
    if (encryptoKeyBinary.empty()) {
        return false;
    }

    snap::utils::crypto::AesEncryptor::Key cryptoKeyData;
    if (encryptoKeyBinary.size() != cryptoKeyData.size()) {
        return false;
    }

    std::memcpy(cryptoKeyData.data(), encryptoKeyBinary.data(), encryptoKeyBinary.size());
    _cryptoKey = {cryptoKeyData};
    return true;
}

void EncryptedDiskCache::generateCryptoKey(const StringBox& path) {
    auto key = DataEncryptor::generateKey();
    _cryptoKey = {key};
    _keychain->store(path,
                     makeShared<ByteBuffer>(reinterpret_cast<const Byte*>(key.data()),
                                            reinterpret_cast<const Byte*>(key.data()) + key.size())
                         ->toBytesView());
}

} // namespace Valdi
