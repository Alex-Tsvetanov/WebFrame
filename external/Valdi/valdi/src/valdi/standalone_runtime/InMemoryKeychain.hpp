//
//  InMemoryKeychain.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/9/20.
//

#pragma once

#include "valdi/Keychain.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"

#include <vector>

namespace Valdi {

class InMemoryKeychain : public snap::valdi::Keychain {
public:
    ~InMemoryKeychain() override;

    /** Store the given string blob into the keychain */
    PlatformResult store(const StringBox& key, const Valdi::BytesView& value) override;

    /**
     * Retrieve the stored blob for the given key.
     * Returns empty string if the value was not found.
     */
    Valdi::BytesView get(const StringBox& key) override;

    /** Erase the stored blob for the given key. */
    bool erase(const StringBox& key) override;

    uint64_t getUpdateSequence() const;

private:
    FlatMap<StringBox, Valdi::BytesView> _blobs;
    mutable Mutex _mutex;
    uint64_t _updateSequence = 0;
};

} // namespace Valdi
