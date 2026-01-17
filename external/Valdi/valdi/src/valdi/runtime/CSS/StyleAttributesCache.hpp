//
//  StyleAttributesCache.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/15/20.
//

#pragma once

#include "valdi/runtime/CSS/CSSAttributes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

#include <vector>

namespace Valdi {

class StyleAttributesCacheKey {
public:
    explicit StyleAttributesCacheKey(const Ref<ValueMap>& rawMap);

    size_t getHash() const;

    bool operator==(const StyleAttributesCacheKey& other) const;

private:
    Ref<ValueMap> _rawMap;
    size_t _hash;
};

class AttributeIds;

/**
 Class which indexes CSSAttributes and can generate a unique id for them.
 */
class StyleAttributesCache {
public:
    explicit StyleAttributesCache(AttributeIds& attributeIds);

    size_t resolveAttributesIndex(const Ref<ValueMap>& attributes);
    Ref<CSSAttributes> getAttributesForIndex(size_t index) const;

    const AttributeIds& getAttributeIds() const;

private:
    FlatMap<StyleAttributesCacheKey, size_t> _entriesLookup;
    std::vector<Ref<CSSAttributes>> _entries;
    AttributeIds& _attributeIds;
};

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::StyleAttributesCacheKey> {
    std::size_t operator()(const Valdi::StyleAttributesCacheKey& k) const;
};

} // namespace std
