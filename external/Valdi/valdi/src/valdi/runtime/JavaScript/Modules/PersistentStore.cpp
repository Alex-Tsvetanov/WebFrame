//
//  PersistentStore.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/18/19.
//

#include "valdi/runtime/JavaScript/Modules/PersistentStore.hpp"
#include "valdi/runtime/Resources/EncryptedDiskCache.hpp"
#include "valdi/runtime/Resources/UserSession.hpp"
#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace Valdi {

constexpr int64_t kPersistentStoreSaveDelayMs = 50;

PersistentStore::PersistentStore(const StringBox& diskCachePath,
                                 const Ref<IDiskCache>& diskCache,
                                 const Ref<UserSession>& userSession,
                                 const Shared<snap::valdi::Keychain>& keychain,
                                 const Ref<DispatchQueue>& dispatchQueue,
                                 ILogger& logger,
                                 uint64_t maxWeight,
                                 bool disableBatchWrites)
    : _diskCachePath(diskCachePath),
      _diskCache(diskCache),
      _userSession(userSession),
      _keychain(keychain),
      _dispatchQueue(dispatchQueue),
      _logger(logger),
      _disableBatchWrites(disableBatchWrites) {
    _store.setMaxWeight(maxWeight);
    updateActiveDiskStore();
}

PersistentStore::~PersistentStore() = default;

void PersistentStore::store(const StringBox& key,
                            const BytesView& blob,
                            uint64_t ttlSeconds,
                            uint64_t weight,
                            Function<void(Result<Void>)> completion) {
    _dispatchQueue->async(
        [key, blob, ttlSeconds, weight, self = strongRef(this), completion = std::move(completion)]() {
            self->_store.store(key, blob, ttlSeconds, weight);
            self->scheduleSave(completion);
        });
}

void PersistentStore::setMaxWeight(uint64_t maxWeight) {
    _store.setMaxWeight(maxWeight);
}

uint64_t PersistentStore::getMaxWeight() const {
    return _store.getMaxWeight();
}

void PersistentStore::setUserSession(const Ref<UserSession>& userSession) {
    _dispatchQueue->async([self = strongRef(this), userSession]() { self->updateUserSession(userSession); });
}

bool PersistentStore::batchWritesDisabled() const {
    return _disableBatchWrites;
}

void PersistentStore::setBatchWritesDisabled(bool batchWritesDisabled) {
    _disableBatchWrites = batchWritesDisabled;
}

Ref<UserSession> PersistentStore::getUserSession() const {
    Ref<UserSession> userSession;

    _dispatchQueue->sync([&]() { userSession = _userSession; });

    return userSession;
}

void PersistentStore::updateUserSession(const Ref<UserSession>& userSession) {
    if (_userSession == userSession) {
        return;
    }

    if (!_pendingSaves.empty()) {
        doSave();
    }

    _userSession = userSession;
    _store.removeAll();

    updateActiveDiskStore();

    doPopulate();
}

void PersistentStore::updateActiveDiskStore() {
    Path directoryPath(_userSession == nullptr ? STRING_LITERAL("global") : _userSession->getUserId());
    auto activeDiskCache = _diskCache->scopedCache(directoryPath, false);
    if (_keychain != nullptr) {
        // Encrypt when we have a keychain
        activeDiskCache = makeShared<EncryptedDiskCache>(activeDiskCache, _keychain);
    }

    _activeDiskCache = std::move(activeDiskCache);
}

void PersistentStore::fetchAll(
    Function<void(const std::vector<std::pair<StringBox, KeyValueStoreEntry>>&)> completion) {
    _dispatchQueue->async([self = strongRef(this), completion = std::move(completion)]() {
        auto entries = self->_store.fetchAll();
        completion(entries);
    });
}

void PersistentStore::fetch(const StringBox& key, Function<void(Result<BytesView>)> completion) {
    _dispatchQueue->async([key, self = strongRef(this), completion = std::move(completion)]() {
        auto data = self->_store.fetch(key);
        if (data) {
            completion(data.value());
        } else {
            completion(Error(STRING_FORMAT("Did not find item '{}'", key)));
        }
    });
}

void PersistentStore::exists(const StringBox& key, Function<void(bool)> completion) {
    _dispatchQueue->async(
        [key, self = strongRef(this), completion = std::move(completion)]() { completion(self->_store.exists(key)); });
}

void PersistentStore::remove(const StringBox& key, Function<void(Result<Void>)> completion) {
    _dispatchQueue->async([key, self = strongRef(this), completion = std::move(completion)]() {
        if (self->_store.remove(key)) {
            self->scheduleSave(completion);
        } else {
            completion(Error(STRING_FORMAT("Did not find item '{}'", key)));
        }
    });
}

void PersistentStore::removeAll(Function<void(Result<Void>)> completion) {
    _dispatchQueue->async([self = strongRef(this), completion = std::move(completion)]() {
        self->_store.removeAll();
        self->scheduleSave(completion);
    });
}

void PersistentStore::scheduleSave(Function<void(Result<Void>)> completion) {
    auto wasEmpty = _pendingSaves.empty();
    _pendingSaves.emplace_back(std::move(completion));
    if (wasEmpty) {
        if (_disableBatchWrites) {
            doSave();
        } else {
            _dispatchQueue->asyncAfter([self = strongRef(this)]() { self->doSave(); },
                                       std::chrono::milliseconds(kPersistentStoreSaveDelayMs));
        }
    }
}

void PersistentStore::doSave() {
    Result<Void> result;
    auto serializeResult = _store.serialize();

    if (serializeResult) {
        result = _activeDiskCache->store(_diskCachePath, serializeResult.value());
    } else {
        result = serializeResult.moveError();
    }

    auto pendingSaves = std::move(_pendingSaves);
    for (const auto& pendingSave : pendingSaves) {
        pendingSave(result);
    }
}

void PersistentStore::populate() {
    _dispatchQueue->async([self = strongRef(this)]() { self->doPopulate(); });
}

void PersistentStore::onPopulateFailure(const Error& error) {
    VALDI_ERROR(
        _logger, "Failed to populate cache at '{}': {}", _activeDiskCache->getAbsoluteURL(_diskCachePath), error);
    _store.removeAll();
}

void PersistentStore::doPopulate() {
    if (_activeDiskCache->exists(_diskCachePath)) {
        auto result = _activeDiskCache->load(_diskCachePath);
        if (!result) {
            onPopulateFailure(result.error());
            return;
        }

        auto populateResult = _store.populate(result.value());
        if (!populateResult) {
            onPopulateFailure(populateResult.error());
            return;
        }
    }
}

void PersistentStore::setCurrentTimeSeconds(uint64_t timeSeconds) {
    _store.setCurrentTimeSeconds(timeSeconds);
}

VALDI_CLASS_IMPL(PersistentStore)

} // namespace Valdi
