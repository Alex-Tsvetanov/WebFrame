//
//  ReferenceTable.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 4/18/23.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/SequenceIDGenerator.hpp"
#include <shared_mutex>

namespace Valdi {

struct ReferenceTableEntry {
    Valdi::SequenceID id;
    size_t retainCount = 0;
    const char* tag = nullptr;

    inline ReferenceTableEntry() = default;
    inline ReferenceTableEntry(Valdi::SequenceID id, size_t retainCount, const char* tag)
        : id(id), retainCount(retainCount), tag(tag) {}

    bool operator==(const ReferenceTableEntry& other) const;
    bool operator!=(const ReferenceTableEntry& other) const;

    size_t getIndex() const;
};

struct ReferenceTableStats {
    /**
     Holds the size of the underlying references table.
     Starts at 0 and grows progressively as the table is filled up.
     */
    size_t tableSize = 0;

    /**
     How many live references exist in the table.
     */
    size_t activeReferencesCount = 0;

    /**
     An array containing the number of active references per each tag,
     sorted by active references count descending
     */
    std::vector<std::pair<const char*, size_t>> activeReferencesByTag;

    static ReferenceTableStats fromEntries(size_t tableSize, const std::vector<ReferenceTableEntry>& entries);
};

class ReferenceTable : public snap::NonCopyable {
public:
    ReferenceTable();
    ~ReferenceTable();

    class ReadAccess : public snap::NonCopyable {
    public:
        ReadAccess(const ReferenceTable* table, std::shared_mutex& sharedMutex);
        ReadAccess(const ReferenceTable* table, std::mutex& uniqueMutex);
        ~ReadAccess();

        bool contains(SequenceID id) const;
        ReferenceTableEntry getEntry(SequenceID id) const;
        std::vector<ReferenceTableEntry> getAll() const;
        void getAll(std::vector<ReferenceTableEntry>& out) const;
        size_t getTableSize() const;

    private:
        std::shared_lock<std::shared_mutex> _sharedLock;
        std::unique_lock<std::mutex> _uniqueLock;
        const ReferenceTable* _table;
    };

    class WriteAccess : public snap::NonCopyable {
    public:
        WriteAccess(ReferenceTable* table, std::shared_mutex& sharedMutex);
        WriteAccess(ReferenceTable* table, std::mutex& uniqueMutex);
        ~WriteAccess();

        bool contains(SequenceID id) const;
        ReferenceTableEntry getEntry(SequenceID id) const;

        ReferenceTableEntry makeRef(const char* tag) const;

        void retainRef(SequenceID id) const;

        /**
         Release the reference with the given id.
         Return true if the reference is still alive, false otherwise.
         */
        bool releaseRef(SequenceID id) const;

    private:
        std::unique_lock<std::shared_mutex> _sharedLock;
        std::unique_lock<std::mutex> _uniqueLock;
        ReferenceTable* _table;
    };

    void setDisallowMultipleConcurrentReaders(bool disallowMultipleConcurrentReaders);

    ReadAccess readAccess() const;
    WriteAccess writeAccess();

    ReferenceTableStats dumpStats() const;

private:
    mutable std::shared_mutex _sharedMutex;
    mutable std::mutex _uniqueMutex;
    Valdi::SequenceIDGenerator _sequence;
    std::vector<ReferenceTableEntry> _entries;
    bool _disallowMultipleConcurrentReaders = false;

    friend WriteAccess;
    friend ReadAccess;

    ReferenceTableEntry& getEntry(SequenceID id);
    const ReferenceTableEntry& getEntry(SequenceID id) const;
    bool contains(SequenceID id) const;

    size_t toEntryIndex(SequenceID id) const;
    static void checkEntry(SequenceID id, const ReferenceTableEntry& entry);
};

} // namespace Valdi
