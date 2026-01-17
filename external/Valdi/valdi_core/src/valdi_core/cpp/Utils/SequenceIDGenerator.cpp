//
//  SequenceIDGenerator.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/11/19.
//

#include "valdi_core/cpp/Utils/SequenceIDGenerator.hpp"

namespace Valdi {

SequenceID::SequenceID(uint64_t id)
    : _salt(static_cast<uint32_t>((id >> 32) & 0xFFFFFFFF)), _index(static_cast<uint32_t>(id & 0xFFFFFFFF)) {}

SequenceID::SequenceID(uint32_t index, uint32_t salt) : _salt(salt), _index(index) {}

SequenceID::SequenceID() = default;

bool SequenceID::isNull() const {
    return _index == 0 && _salt == 0;
}

uint32_t SequenceID::getSalt() const {
    return _salt;
}

uint64_t SequenceID::getId() const {
    return static_cast<uint64_t>(_salt) << 32 | static_cast<uint64_t>(_index);
}

uint32_t SequenceID::getIndex() const {
    return _index;
}

size_t SequenceID::getIndexAsSize() const {
    return static_cast<size_t>(_index);
}

bool SequenceID::operator==(const SequenceID& other) const {
    return _index == other._index && _salt == other._salt;
}

bool SequenceID::operator!=(const SequenceID& other) const {
    return !(*this == other);
}

uint32_t SequenceIDGenerator::nextIndex() {
    if (!_freeIndexes.empty()) {
        auto index = _freeIndexes.back();
        _freeIndexes.pop_back();
        return index;
    }
    return _indexSequence++;
}

uint32_t SequenceIDGenerator::nextSalt() {
    return ++_saltSequence;
}

SequenceID SequenceIDGenerator::newId() {
    return SequenceID(nextIndex(), nextSalt());
}

void SequenceIDGenerator::releaseId(SequenceID id) {
    _freeIndexes.emplace_back(id.getIndex());
}

void SequenceIDGenerator::releaseId(uint64_t id) {
    releaseId(SequenceID(id));
}

void SequenceIDGenerator::reset() {
    _freeIndexes = {};
    _indexSequence = 0;
    _saltSequence = 0;
}

} // namespace Valdi
