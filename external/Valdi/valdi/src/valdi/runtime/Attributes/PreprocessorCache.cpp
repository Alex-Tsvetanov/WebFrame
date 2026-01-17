//
//  PreprocessorCache.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/4/19.
//

#include "valdi/runtime/Attributes/PreprocessorCache.hpp"

namespace Valdi {

PreprocessorCacheValue::PreprocessorCacheValue(const PreprocessorCacheKey& key,
                                               const Value& value,
                                               Weak<PreprocessorCache> cache)
    : _key(key), _value(value), _cache(std::move(cache)) {}

PreprocessorCacheValue::~PreprocessorCacheValue() {
    auto cache = _cache.lock();
    if (cache != nullptr) {
        cache->removeCacheKey(_key);
    }
}

const Value& PreprocessorCacheValue::value() const {
    return _value;
}

PreprocessedValue::PreprocessedValue(Value value) : value(std::move(value)) {}

PreprocessedValue::PreprocessedValue(Value value, Ref<SharedPtrRefCountable> handle)
    : value(std::move(value)), handle(std::move(handle)) {}

std::optional<PreprocessedValue> PreprocessorCache::get(const PreprocessorCacheKey& key) const {
    std::lock_guard<Mutex> guard(_mutex);

    const auto& it = _cache.find(key);
    if (it == _cache.end()) {
        return std::nullopt;
    }

    auto strongCachedValue = it->second.lock();
    if (strongCachedValue == nullptr) {
        return std::nullopt;
    }

    auto value = strongCachedValue->value();

    return PreprocessedValue(std::move(value), strongCachedValue);
}

PreprocessedValue PreprocessorCache::store(const PreprocessorCacheKey& key, const Value& value) {
    std::lock_guard<Mutex> guard(_mutex);
    auto cachedValue = Valdi::makeShared<PreprocessorCacheValue>(key, value, weak_from_this());
    _cache[key] = cachedValue;
    return PreprocessedValue(value, cachedValue);
}

void PreprocessorCache::removeCacheKey(const PreprocessorCacheKey& key) {
    std::lock_guard<Mutex> guard(_mutex);
    const auto& it = _cache.find(key);
    if (it != _cache.end()) {
        _cache.erase(key);
    }
}

void PreprocessorCache::clear() {
    std::lock_guard<Mutex> guard(_mutex);
    _cache.clear();
}

} // namespace Valdi
