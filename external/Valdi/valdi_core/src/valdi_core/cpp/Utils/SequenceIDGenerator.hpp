//
//  SequenceIDGenerator.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/11/19.
//

#pragma once

#include <vector>

namespace Valdi {

class SequenceID {
public:
    explicit SequenceID(uint64_t id);
    SequenceID(uint32_t index, uint32_t salt);
    SequenceID();

    uint32_t getSalt() const;
    uint32_t getIndex() const;
    uint64_t getId() const;

    size_t getIndexAsSize() const;

    bool isNull() const;

    bool operator==(const SequenceID& other) const;
    bool operator!=(const SequenceID& other) const;

private:
    uint32_t _salt = 0;
    uint32_t _index = 0;
};

/**
 A class that generates unique id. Each id has an index
 which is suitable for inserting into a contiguous array.
 Ids can be released, which makes the index available for
 a next newId() call. Each id has a unique salt generated
 with it, so that ids from the same index won't match.
 This design makes those ids suitable from array insertion
 while providing the uniqueness and safety of a purely
 increasing sequence.

 releaseId() needs to be called only once, the generator
 does not offer protection against indexes released more
 than once.
 */
class SequenceIDGenerator {
public:
    SequenceID newId();

    void releaseId(uint64_t id);
    void releaseId(SequenceID id);

    void reset();

private:
    std::vector<uint32_t> _freeIndexes;
    // Only increases when there are no free indexes
    uint32_t _indexSequence = 0;
    // Increases every time, thus guaranteeing the uniqueness
    // of each id.
    uint32_t _saltSequence = 0;

    uint32_t nextIndex();
    uint32_t nextSalt();
};
} // namespace Valdi
