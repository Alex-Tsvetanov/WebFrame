//
//  KeyValueStore.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 01/26/24.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <optional>

namespace Valdi {

class ByteBuffer;

struct KeyValueStoreEntry {
    uint64_t mutationId;
    uint64_t expirationDate;
    uint64_t weight;
    BytesView data;

    KeyValueStoreEntry(uint64_t mutationId, uint64_t expirationDate, uint64_t weight, const BytesView& data);
    KeyValueStoreEntry();
};

class KeyValueStore {
public:
    KeyValueStore();
    ~KeyValueStore();

    Result<BytesView> serialize();
    Result<Void> populate(const BytesView& data);

    void store(const StringBox& key, const BytesView& blob, uint64_t ttlSeconds, uint64_t weight);

    std::optional<BytesView> fetch(const StringBox& key);
    std::vector<std::pair<StringBox, KeyValueStoreEntry>> fetchAll();

    bool exists(const StringBox& key);
    bool remove(const StringBox& key);
    void removeAll();

    uint64_t getMaxWeight() const;
    void setMaxWeight(uint64_t maxWeight);

    uint64_t getMutationId() const;

    // For unit tests
    void setCurrentTimeSeconds(uint64_t timeSeconds);

private:
    uint64_t _currentTimeSeconds = 0;
    uint64_t _mutationId = 0;
    uint64_t _maxWeight;
    FlatMap<StringBox, KeyValueStoreEntry> _entries;

    void buildManifest(const std::vector<std::pair<StringBox, KeyValueStoreEntry>>& entries, ByteBuffer& output) const;
    std::vector<std::pair<StringBox, KeyValueStoreEntry>> collectEntries();
    void evictEntriesIfNeeded(std::vector<std::pair<StringBox, KeyValueStoreEntry>>& collectedEntries);

    bool isEntryExpired(const KeyValueStoreEntry& entry) const;

    uint64_t currentTimeSeconds() const;

    std::optional<BytesView> fetch(const StringBox& key, bool updateSequence);
};

} // namespace Valdi
