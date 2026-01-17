//
//  EncryptedDiskCache.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 1/30/24.
//

#pragma once

#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi/runtime/Utils/DataEncryptor.hpp"

#include "valdi_core/cpp/Utils/Mutex.hpp"

#include <mutex>

namespace snap::valdi {
class Keychain;
}

namespace Valdi {

/**
 EncryptedDiskCache is an implementation of DiskCache which takes another DiskCache instance to perform
 the actual load, but will encrypt data before writing into the disk cache and decrypt when returning data from
 the disk cache. It will generate and store a private key into the given keychain.
 */
class EncryptedDiskCache : public IDiskCache {
public:
    EncryptedDiskCache(const Ref<IDiskCache>& diskCache, const Shared<snap::valdi::Keychain>& keychain);
    ~EncryptedDiskCache() override;

    bool exists(const Path& path) final;

    Result<BytesView> load(const Path& path) final;

    Result<BytesView> loadForAbsoluteURL(const StringBox& url) final;

    Result<Void> store(const Path& path, const BytesView& bytes) final;

    bool remove(const Path& path) final;

    StringBox getAbsoluteURL(const Path& path) const final;

    Ref<IDiskCache> scopedCache(const Path& path, bool allowsReadOutsideOfScope) const final;

    Path getRootPath() const final;

    std::vector<Path> list(const Path& path) const final;

    void generateOrRestoreCryptoKey();

    static StringBox getKeychainKey();

private:
    Mutex _mutex;
    Ref<IDiskCache> _diskCache;
    Shared<snap::valdi::Keychain> _keychain;
    std::optional<snap::utils::crypto::AesEncryptor::Key> _cryptoKey;

    EncryptedDiskCache(const Ref<IDiskCache>& diskCache,
                       const Shared<snap::valdi::Keychain>& keychain,
                       const std::optional<snap::utils::crypto::AesEncryptor::Key>& cryptoKey);

    Result<BytesView> decrypt(const BytesView& input);
    Result<BytesView> encrypt(const BytesView& input);

    DataEncryptor getDataEncryptor();
    bool restoreCryptoKey(const StringBox& path);
    void generateCryptoKey(const StringBox& path);
};

} // namespace Valdi
