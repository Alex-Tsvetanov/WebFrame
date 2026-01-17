//
//  TextShaperCache.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/15/22.
//

#include "snap_drawing/cpp/Text/TextShaperCache.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include <boost/functional/hash.hpp>

namespace snap::drawing {

static size_t computeHash(FontId fontId,
                          Scalar letterSpacing,
                          TextScript script,
                          bool isRightToLeft,
                          const Character* characters,
                          size_t length) {
    auto stringView = std::u32string_view(reinterpret_cast<const char32_t*>(characters), length);

    auto hash = static_cast<size_t>(fontId);
    boost::hash_combine(hash, std::hash<float>()(letterSpacing));
    boost::hash_combine(hash, std::hash<uint32_t>()(script.code));
    boost::hash_combine(hash, std::hash<bool>()(isRightToLeft));
    boost::hash_combine(hash, std::hash<std::u32string_view>()(stringView));

    return hash;
}

TextShaperCacheKey::TextShaperCacheKey(FontId fontId,
                                       Scalar letterSpacing,
                                       TextScript script,
                                       bool isRightToLeft,
                                       const Character* characters,
                                       size_t length)
    : TextShaperCacheKey(fontId,
                         letterSpacing,
                         script,
                         isRightToLeft,
                         characters,
                         length,
                         computeHash(fontId, letterSpacing, script, isRightToLeft, characters, length)) {}

bool TextShaperCacheKey::operator==(const TextShaperCacheKey& other) const {
    if (fontId != other.fontId || letterSpacing != other.letterSpacing ||
        script != other.script != isRightToLeft != other.isRightToLeft || length != other.length) {
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (characters[i] != other.characters[i]) {
            return false;
        }
    }

    return true;
}

bool TextShaperCacheKey::operator!=(const TextShaperCacheKey& other) const {
    return !(*this == other);
}

TextShaperCacheNode::TextShaperCacheNode(std::pair<TextShaperCacheKey, TextShaperCacheValue>&& keyValue)
    : Valdi::LRUCacheNode<TextShaperCacheKey, TextShaperCacheValue>(std::move(keyValue)) {}

Ref<TextShaperCacheNode> TextShaperCacheNode::make(FontId fontId,
                                                   Scalar letterSpacing,
                                                   TextScript script,
                                                   bool isRightToLeft,
                                                   const Character* characters,
                                                   size_t charactersLength,
                                                   const ShapedGlyph* glyphs,
                                                   size_t glyphsLength,
                                                   size_t hash) {
    /**
     This does a single allocation containing the TextShaperCacheNode structure, the characters array, and the shaped
     glyphs.
     */

    auto charactersAllocSize = charactersLength * sizeof(Character);
    auto glyphsAllocSize = glyphsLength * sizeof(ShapedGlyph);

    auto baseAllocSize = sizeof(TextShaperCacheNode);
    auto withCharactersAllocSize = Valdi::alignUp(baseAllocSize + charactersAllocSize, alignof(Character));
    auto withGlyphsAllocSize = Valdi::alignUp(withCharactersAllocSize + glyphsAllocSize, alignof(ShapedGlyph));

    std::allocator<uint8_t> allocator;
    auto* region = allocator.allocate(withGlyphsAllocSize);

    auto* allocatedCharacters = reinterpret_cast<Character*>(&region[withCharactersAllocSize - charactersAllocSize]);
    auto* allocatedGlyphs = reinterpret_cast<ShapedGlyph*>(&region[withGlyphsAllocSize - glyphsAllocSize]);

    if (characters != nullptr) {
        std::memcpy(allocatedCharacters, characters, charactersAllocSize);
    }
    if (glyphs != nullptr) {
        std::memcpy(allocatedGlyphs, glyphs, glyphsAllocSize);
    }

    auto cacheKey =
        TextShaperCacheKey(fontId, letterSpacing, script, isRightToLeft, allocatedCharacters, charactersLength, hash);
    auto cacheValue = TextShaperCacheValue(allocatedGlyphs, glyphsLength);

    new (region)(TextShaperCacheNode)(std::make_pair(cacheKey, cacheValue));

    return Ref<TextShaperCacheNode>(reinterpret_cast<TextShaperCacheNode*>(region));
}

TextShaperCache::TextShaperCache(size_t capacity) : _cache(capacity) {}

TextShaperCache::~TextShaperCache() = default;

void TextShaperCache::clear() {
    _cache.clear();
}

bool TextShaperCache::contains(const TextShaperCacheKey& key) const {
    return _cache.contains(key);
}

std::optional<TextShaperCacheValue> TextShaperCache::find(const TextShaperCacheKey& key) {
    const auto& it = _cache.find(key);
    if (it == _cache.end()) {
        return std::nullopt;
    }

    return {it->value()};
}

void TextShaperCache::insert(const TextShaperCacheKey& key, const ShapedGlyph* glyphs, size_t glyphsLength) {
    auto node = TextShaperCacheNode::make(key.fontId,
                                          key.letterSpacing,
                                          key.script,
                                          key.isRightToLeft,
                                          key.characters,
                                          key.length,
                                          glyphs,
                                          glyphsLength,
                                          key.hash());
    _cache.insert(node);
}

} // namespace snap::drawing

namespace std {

std::size_t hash<snap::drawing::TextShaperCacheKey>::operator()(
    const snap::drawing::TextShaperCacheKey& k) const noexcept {
    return k.hash();
}

} // namespace std
