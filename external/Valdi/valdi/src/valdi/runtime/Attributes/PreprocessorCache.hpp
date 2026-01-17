//
//  PreprocessorCache.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/4/19.
//

#pragma once

#include "valdi/runtime/Attributes/PreprocessorCacheKey.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include <optional>

namespace Valdi {

/**
 Represents a preprocessed Value with an opaque handle
 which should be kept by the consumer as long as the preprocessed
 value should be cached. Whenever the handle is released, the
 preprocessed value will be released from the cache.
 */
struct PreprocessedValue {
    Value value;
    Ref<SharedPtrRefCountable> handle;

    explicit PreprocessedValue(Value value);
    PreprocessedValue(Value value, Ref<SharedPtrRefCountable> handle);
};

class PreprocessorCache;
class PreprocessorCacheValue : public SharedPtrRefCountable {
public:
    PreprocessorCacheValue(const PreprocessorCacheKey& key, const Value& value, Weak<PreprocessorCache> cache);
    ~PreprocessorCacheValue() override;

    const Value& value() const;

private:
    PreprocessorCacheKey _key;
    Value _value;
    Weak<PreprocessorCache> _cache;
};

class PreprocessorCache : public std::enable_shared_from_this<PreprocessorCache> {
public:
    std::optional<PreprocessedValue> get(const PreprocessorCacheKey& key) const;
    PreprocessedValue store(const PreprocessorCacheKey& key, const Value& value);
    void clear();

private:
    FlatMap<PreprocessorCacheKey, Weak<PreprocessorCacheValue>> _cache;
    mutable Mutex _mutex;

    friend PreprocessorCacheValue;
    void removeCacheKey(const PreprocessorCacheKey& key);
};

} // namespace Valdi
