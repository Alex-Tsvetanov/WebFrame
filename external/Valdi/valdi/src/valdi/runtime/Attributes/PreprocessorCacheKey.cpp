//
//  PreprocessorCacheKey.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/4/19.
//

#include "valdi/runtime/Attributes/PreprocessorCacheKey.hpp"

namespace Valdi {

PreprocessorCacheKey::PreprocessorCacheKey(const Value& value) : _value(value), _hash(value.hash()) {}

size_t PreprocessorCacheKey::hash() const {
    return _hash;
}

bool PreprocessorCacheKey::operator==(const PreprocessorCacheKey& other) const {
    return _value == other._value;
}

bool PreprocessorCacheKey::operator!=(const PreprocessorCacheKey& other) const {
    return !(*this == other);
}

} // namespace Valdi

namespace std {

std::size_t hash<Valdi::PreprocessorCacheKey>::operator()(const Valdi::PreprocessorCacheKey& k) const noexcept {
    return k.hash();
}

} // namespace std
