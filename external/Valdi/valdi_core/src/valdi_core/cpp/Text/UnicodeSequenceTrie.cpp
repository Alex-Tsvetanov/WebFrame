#include "valdi_core/cpp/Text/UnicodeSequenceTrie.hpp"

namespace Valdi {

UnicodeSequenceTrie::UnicodeSequenceTrie() = default;
UnicodeSequenceTrie::~UnicodeSequenceTrie() = default;

void UnicodeSequenceTrie::insert(const uint32_t* sequence, size_t length) {
    emplace(_rootEntry, 0, sequence, length);
}

void UnicodeSequenceTrie::insert(uint32_t unicodePoint) {
    insert(&unicodePoint, 1);
}

void UnicodeSequenceTrie::insert(std::initializer_list<uint32_t> sequence) {
    insert(sequence.begin(), sequence.size());
}

const UnicodeSequence* UnicodeSequenceTrie::find(const uint32_t* unicodePoints, size_t length) const {
    return find(unicodePoints, unicodePoints + length);
}

const UnicodeSequence* UnicodeSequenceTrie::find(const uint32_t* unicodePointsBegin,
                                                 const uint32_t* unicodePointsEnd) const {
    const UnicodeSequence* lastFoundSequence = nullptr;
    const auto* currentEntry = &_rootEntry;

    while (unicodePointsBegin != unicodePointsEnd) {
        auto unicodePoint = *unicodePointsBegin;

        if (!currentEntry->characterSet->contains(unicodePoint)) {
            break;
        }

        currentEntry = &currentEntry->children.find(unicodePoint)->second;

        if (currentEntry->exists) {
            lastFoundSequence = &currentEntry->unicodeSequence;
        }

        unicodePointsBegin++;
    }

    return lastFoundSequence;
}

const UnicodeSequence* UnicodeSequenceTrie::find(uint32_t unicodePoint) const {
    return find(&unicodePoint, 1);
}

const UnicodeSequence* UnicodeSequenceTrie::find(std::initializer_list<uint32_t> unicodePoints) const {
    return find(unicodePoints.begin(), unicodePoints.size());
}

void UnicodeSequenceTrie::rebuildIndex() {
    buildIndex(_rootEntry);
}

void UnicodeSequenceTrie::buildIndex(UnicodeSequenceTrie::Entry& entry) {
    if (entry.characterSet == nullptr) {
        std::vector<CharacterRange> characters = {};
        characters.reserve(entry.children.size());

        for (const auto& it : entry.children) {
            characters.emplace_back(it.first, it.first);
        }

        entry.characterSet = CharacterSet::make(characters.data(), characters.size());
    }

    for (auto& it : entry.children) {
        buildIndex(it.second);
    }
}

UnicodeSequenceTrie::Entry& UnicodeSequenceTrie::emplace(UnicodeSequenceTrie::Entry& current,
                                                         size_t index,
                                                         const uint32_t* sequence,
                                                         size_t length) {
    if (index == length) {
        current.exists = true;
        return current;
    }

    auto unicodePoint = sequence[index];

    auto it = current.children.find(unicodePoint);
    if (it == current.children.end()) {
        // Needs to be rebuild
        current.characterSet = nullptr;

        auto& entry = current.children[unicodePoint];
        for (size_t i = 0; i <= index; i++) {
            entry.unicodeSequence.emplace_back(sequence[i]);
        }

        return emplace(entry, index + 1, sequence, length);
    } else {
        return emplace(it->second, index + 1, sequence, length);
    }
}

} // namespace Valdi
