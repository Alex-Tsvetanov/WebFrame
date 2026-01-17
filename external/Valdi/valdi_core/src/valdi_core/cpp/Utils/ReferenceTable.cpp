//
//  ReferenceTable.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 4/18/23.
//

#include "valdi_core/cpp/Utils/ReferenceTable.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include <algorithm>
#include <fmt/format.h>

namespace Valdi {

ReferenceTableStats ReferenceTableStats::fromEntries(size_t tableSize,
                                                     const std::vector<ReferenceTableEntry>& entries) {
    ReferenceTableStats out;
    out.tableSize = tableSize;
    out.activeReferencesCount = entries.size();

    Valdi::FlatMap<const char*, size_t> activeReferencesByTag;

    for (const auto& entry : entries) {
        auto it = activeReferencesByTag.find(entry.tag);
        if (it == activeReferencesByTag.end()) {
            activeReferencesByTag[entry.tag] = 1;
        } else {
            it->second++;
        }
    }

    for (const auto& it : activeReferencesByTag) {
        out.activeReferencesByTag.emplace_back(it);
    }

    std::sort(out.activeReferencesByTag.begin(),
              out.activeReferencesByTag.end(),
              [](const std::pair<const char*, size_t>& left, const std::pair<const char*, size_t>& right) -> bool {
                  return left.second > right.second;
              });

    return out;
}

static void handleFatalError(const std::string& message) {
    SC_ABORT(message);
}

bool ReferenceTableEntry::operator==(const ReferenceTableEntry& other) const {
    return id == other.id && retainCount == other.retainCount && tag == other.tag;
}

bool ReferenceTableEntry::operator!=(const ReferenceTableEntry& other) const {
    return !(*this == other);
}

size_t ReferenceTableEntry::getIndex() const {
    return static_cast<size_t>(id.getIndex());
}

ReferenceTable::ReferenceTable() = default;
ReferenceTable::~ReferenceTable() = default;

void ReferenceTable::setDisallowMultipleConcurrentReaders(bool disallowMultipleConcurrentReaders) {
    _disallowMultipleConcurrentReaders = disallowMultipleConcurrentReaders;
}

ReferenceTable::WriteAccess ReferenceTable::writeAccess() {
    if (_disallowMultipleConcurrentReaders) {
        return WriteAccess(this, _uniqueMutex);
    } else {
        return WriteAccess(this, _sharedMutex);
    }
}

ReferenceTable::ReadAccess ReferenceTable::readAccess() const {
    if (_disallowMultipleConcurrentReaders) {
        return ReadAccess(this, _uniqueMutex);
    } else {
        return ReadAccess(this, _sharedMutex);
    }
}

ReferenceTableStats ReferenceTable::dumpStats() const {
    size_t tableSize;
    std::vector<ReferenceTableEntry> entries;
    {
        auto read = readAccess();
        tableSize = read.getTableSize();
        entries = read.getAll();
    }

    return ReferenceTableStats::fromEntries(tableSize, entries);
}

bool ReferenceTable::contains(SequenceID id) const {
    auto index = toEntryIndex(id);
    const auto& entry = _entries[index];
    return entry.retainCount != 0 && entry.id == id;
}

ReferenceTableEntry& ReferenceTable::getEntry(SequenceID id) {
    auto index = toEntryIndex(id);

    auto& entry = _entries[index];
    checkEntry(id, entry);

    return entry;
}

const ReferenceTableEntry& ReferenceTable::getEntry(SequenceID id) const {
    auto index = toEntryIndex(id);

    const auto& entry = _entries[index];
    checkEntry(id, entry);

    return entry;
}

size_t ReferenceTable::toEntryIndex(SequenceID id) const {
    auto index = static_cast<size_t>(id.getIndex());
    if (index >= _entries.size()) {
        handleFatalError(
            fmt::format("Out of bounds reference, max size is {}, provided index in id is {}", _entries.size(), index));
    }

    return index;
}

void ReferenceTable::checkEntry(SequenceID id, const ReferenceTableEntry& entry) {
    if (entry.retainCount == 0 || entry.id != id) {
        handleFatalError(fmt::format("Invalid reference id {}x{}: entry is used by {}x{}, retainCount {}, tag {}",
                                     id.getSalt(),
                                     id.getIndex(),
                                     entry.id.getSalt(),
                                     entry.id.getIndex(),
                                     entry.retainCount,
                                     entry.tag != nullptr ? entry.tag : "<null>"));
    }
}

ReferenceTable::ReadAccess::ReadAccess(const ReferenceTable* table, std::shared_mutex& sharedMutex)
    : _sharedLock(sharedMutex), _table(table) {}
ReferenceTable::ReadAccess::ReadAccess(const ReferenceTable* table, std::mutex& uniqueMutex)
    : _uniqueLock(uniqueMutex), _table(table) {}
ReferenceTable::ReadAccess::~ReadAccess() = default;

bool ReferenceTable::ReadAccess::contains(SequenceID id) const {
    return _table->contains(id);
}

ReferenceTableEntry ReferenceTable::ReadAccess::getEntry(SequenceID id) const {
    return _table->getEntry(id);
}

std::vector<ReferenceTableEntry> ReferenceTable::ReadAccess::getAll() const {
    std::vector<ReferenceTableEntry> entries;
    getAll(entries);
    return entries;
}

void ReferenceTable::ReadAccess::getAll(std::vector<ReferenceTableEntry>& out) const {
    out.reserve(_table->_entries.size());

    for (const auto& entry : _table->_entries) {
        if (entry.retainCount > 0) {
            out.emplace_back(entry);
        }
    }
}

size_t ReferenceTable::ReadAccess::getTableSize() const {
    return _table->_entries.size();
}

ReferenceTable::WriteAccess::WriteAccess(ReferenceTable* table, std::shared_mutex& sharedMutex)
    : _sharedLock(sharedMutex), _table(table) {}
ReferenceTable::WriteAccess::WriteAccess(ReferenceTable* table, std::mutex& uniqueMutex)
    : _uniqueLock(uniqueMutex), _table(table) {}
ReferenceTable::WriteAccess::~WriteAccess() = default;

ReferenceTableEntry ReferenceTable::WriteAccess::makeRef(const char* tag) const {
    auto id = _table->_sequence.newId();

    auto index = static_cast<size_t>(id.getIndex());
    ReferenceTableEntry* entry;
    if (index >= _table->_entries.size()) {
        entry = &_table->_entries.emplace_back();
    } else {
        entry = &_table->_entries[index];
    }

    entry->id = id;
    entry->retainCount = 1;
    entry->tag = tag;

    return *entry;
}

void ReferenceTable::WriteAccess::retainRef(SequenceID id) const {
    auto& entry = _table->getEntry(id);

    entry.retainCount++;
}

bool ReferenceTable::WriteAccess::releaseRef(SequenceID id) const {
    auto& entry = _table->getEntry(id);
    entry.retainCount--;

    if (entry.retainCount > 0) {
        return true;
    }

    entry.id = Valdi::SequenceID();
    entry.tag = nullptr;
    _table->_sequence.releaseId(id);
    return false;
}

bool ReferenceTable::WriteAccess::contains(SequenceID id) const {
    return _table->contains(id);
}

ReferenceTableEntry ReferenceTable::WriteAccess::getEntry(SequenceID id) const {
    return _table->getEntry(id);
}

} // namespace Valdi
