//
//  PreprocessorCacheKey.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/4/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class PreprocessorCacheKey {
public:
    explicit PreprocessorCacheKey(const Value& value);

    size_t hash() const;
    const Value& value() const;

    bool operator==(const PreprocessorCacheKey& other) const;
    bool operator!=(const PreprocessorCacheKey& other) const;

private:
    Value _value;
    size_t _hash = 0;
};

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::PreprocessorCacheKey> {
    std::size_t operator()(const Valdi::PreprocessorCacheKey& k) const noexcept;
};

} // namespace std
