//
//  EncryptedDiskCache_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 1/30/24.
//

#include "valdi/runtime/Resources/EncryptedDiskCache.hpp"

#include "valdi/standalone_runtime/InMemoryDiskCache.hpp"
#include "valdi/standalone_runtime/InMemoryKeychain.hpp"

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include "gtest/gtest.h"

using namespace Valdi;

namespace {

static BytesView makeData(std::string_view str) {
    return makeShared<ByteBuffer>(str)->toBytesView();
}

TEST(EncryptedDiskCache, generatesKeyWhenKeychainEmpty) {
    auto innerDiskCache = makeShared<InMemoryDiskCache>();
    auto keychain = makeShared<InMemoryKeychain>();
    auto diskCache = makeShared<EncryptedDiskCache>(innerDiskCache, keychain);

    ASSERT_TRUE(keychain->get(EncryptedDiskCache::getKeychainKey()).empty());

    auto result = diskCache->store(Path("file.txt"), makeData("Hello World"));
    ASSERT_TRUE(result) << result.description();

    ASSERT_FALSE(keychain->get(EncryptedDiskCache::getKeychainKey()).empty());
}

TEST(EncryptedDiskCache, restoresKeyWhenKeychainNonEmpty) {
    auto innerDiskCache = makeShared<InMemoryDiskCache>();
    auto keychain = makeShared<InMemoryKeychain>();

    {
        auto diskCache = makeShared<EncryptedDiskCache>(innerDiskCache, keychain);
        diskCache->generateOrRestoreCryptoKey();
    }

    auto cryptoKey = keychain->get(EncryptedDiskCache::getKeychainKey());

    ASSERT_FALSE(cryptoKey.empty());

    auto diskCache = makeShared<EncryptedDiskCache>(innerDiskCache, keychain);
    auto result = diskCache->store(Path("file.txt"), makeData("Hello World"));
    ASSERT_TRUE(result) << result.description();

    auto cryptoKeyAfter = keychain->get(EncryptedDiskCache::getKeychainKey());

    ASSERT_FALSE(cryptoKeyAfter.empty());

    ASSERT_EQ(cryptoKey, cryptoKeyAfter);
}

TEST(EncryptedDiskCache, storesEncrypted) {
    auto innerDiskCache = makeShared<InMemoryDiskCache>();
    auto keychain = makeShared<InMemoryKeychain>();
    auto diskCache = makeShared<EncryptedDiskCache>(innerDiskCache, keychain);

    auto result = diskCache->store(Path("file.txt"), makeData("Hello World"));
    ASSERT_TRUE(result) << result.description();

    auto data = innerDiskCache->load(Path("file.txt"));
    ASSERT_TRUE(data) << data.description();

    ASSERT_NE("Hello World", data.value().asStringView());
}

TEST(EncryptedDiskCache, decryptsOnLoad) {
    auto innerDiskCache = makeShared<InMemoryDiskCache>();
    auto keychain = makeShared<InMemoryKeychain>();
    auto diskCache = makeShared<EncryptedDiskCache>(innerDiskCache, keychain);

    auto result = diskCache->store(Path("file.txt"), makeData("Hello World"));
    ASSERT_TRUE(result) << result.description();

    auto data = diskCache->load(Path("file.txt"));
    ASSERT_TRUE(data) << data.description();

    ASSERT_EQ("Hello World", data.value().asStringView());
}

} // namespace
