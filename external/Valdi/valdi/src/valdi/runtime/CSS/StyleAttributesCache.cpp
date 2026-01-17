//
//  StyleAttributesCache.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/15/20.
//

#include "valdi/runtime/CSS/StyleAttributesCache.hpp"
#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

namespace Valdi {

StyleAttributesCache::StyleAttributesCache(AttributeIds& attributeIds) : _attributeIds(attributeIds) {}

size_t StyleAttributesCache::resolveAttributesIndex(const Ref<ValueMap>& attributes) {
    auto key = StyleAttributesCacheKey(attributes);
    const auto& it = _entriesLookup.find(key);
    if (it != _entriesLookup.end()) {
        return it->second;
    }

    std::vector<CSSStyleDeclaration> declarations;
    declarations.reserve(attributes->size());
    for (const auto& it : *attributes) {
        auto& decl = declarations.emplace_back();
        decl.attribute.id = _attributeIds.getIdForName(it.first);
        decl.attribute.value = it.second;
    }

    auto cssAttributes = makeShared<CSSAttributes>(std::move(declarations));
    auto index = _entries.size();

    _entries.emplace_back(cssAttributes);
    _entriesLookup[key] = index;

    return index;
}

Ref<CSSAttributes> StyleAttributesCache::getAttributesForIndex(size_t index) const {
    if (index >= _entries.size()) {
        return nullptr;
    }
    return _entries[index];
}

const AttributeIds& StyleAttributesCache::getAttributeIds() const {
    return _attributeIds;
}

StyleAttributesCacheKey::StyleAttributesCacheKey(const Ref<ValueMap>& rawMap)
    : _rawMap(rawMap), _hash(Value(rawMap).hash()) {}

size_t StyleAttributesCacheKey::getHash() const {
    return _hash;
}

bool StyleAttributesCacheKey::operator==(const StyleAttributesCacheKey& other) const {
    return *_rawMap == *other._rawMap;
}

} // namespace Valdi

namespace std {

std::size_t hash<Valdi::StyleAttributesCacheKey>::operator()(const Valdi::StyleAttributesCacheKey& k) const {
    return k.getHash();
}

} // namespace std
