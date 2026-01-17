//
//  PersistentStore.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/18/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi/runtime/Resources/KeyValueStore.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

#include "utils/crypto/AesEncryptor.hpp"

#include <optional>

namespace snap::valdi {
class Keychain;
}

namespace Valdi {

class UserSession;

class PersistentStore : public ValdiObject {
public:
    PersistentStore(const StringBox& diskCachePath,
                    const Ref<IDiskCache>& diskCache,
                    const Ref<UserSession>& userSession,
                    const Shared<snap::valdi::Keychain>& keychain,
                    const Ref<DispatchQueue>& dispatchQueue,
                    ILogger& logger,
                    uint64_t maxWeight,
                    bool disableBatchWrites);
    ~PersistentStore() override;

    void populate();

    uint64_t getMaxWeight() const;
    void setMaxWeight(uint64_t maxWeight);

    Ref<UserSession> getUserSession() const;
    void setUserSession(const Ref<UserSession>& userSession);

    bool batchWritesDisabled() const;
    void setBatchWritesDisabled(bool batchWritesDisabled);

    void store(const StringBox& key,
               const BytesView& blob,
               uint64_t ttlSeconds,
               uint64_t weight,
               Function<void(Result<Void>)> completion);
    void fetch(const StringBox& key, Function<void(Result<BytesView>)> completion);
    void fetchAll(Function<void(const std::vector<std::pair<StringBox, KeyValueStoreEntry>>&)> completion);
    void exists(const StringBox& key, Function<void(bool)> completion);
    void remove(const StringBox& key, Function<void(Result<Void>)> completion);
    void removeAll(Function<void(Result<Void>)> completion);

    // For unit tests
    void setCurrentTimeSeconds(uint64_t timeSeconds);

    VALDI_CLASS_HEADER(PersistentStore)

private:
    KeyValueStore _store;
    Path _diskCachePath;
    Ref<IDiskCache> _diskCache;
    Ref<IDiskCache> _activeDiskCache;
    Ref<UserSession> _userSession;
    Shared<snap::valdi::Keychain> _keychain;
    Ref<DispatchQueue> _dispatchQueue;
    [[maybe_unused]] ILogger& _logger;
    bool _disableBatchWrites;
    std::vector<Function<void(Result<Void>)>> _pendingSaves;

    void scheduleSave(Function<void(Result<Void>)> completion);
    void doSave();
    void doPopulate();

    void updateUserSession(const Ref<UserSession>& userSession);
    void updateActiveDiskStore();

    void onPopulateFailure(const Error& error);
};

} // namespace Valdi
