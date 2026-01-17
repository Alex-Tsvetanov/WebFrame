//
//  TextShaperCache.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/15/22.
//

#pragma once

#include "valdi_core/cpp/Utils/LRUCache.hpp"

#include "snap_drawing/cpp/Text/Character.hpp"
#include "snap_drawing/cpp/Text/Font.hpp"
#include "snap_drawing/cpp/Text/TextShaper.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

#include <optional>

namespace snap::drawing {

struct TextShaperCacheKey {
    FontId fontId = 0;
    Scalar letterSpacing = 0.0f;
    TextScript script;
    bool isRightToLeft = false;
    const Character* characters = nullptr;
    size_t length = 0;

    constexpr TextShaperCacheKey() = default;

    constexpr TextShaperCacheKey(FontId fontId,
                                 Scalar letterSpacing,
                                 TextScript script,
                                 bool isRightToLeft,
                                 const Character* characters,
                                 size_t length,
                                 size_t hash)
        : fontId(fontId),
          letterSpacing(letterSpacing),
          script(script),
          isRightToLeft(isRightToLeft),
          characters(characters),
          length(length),
          _hash(hash) {}

    TextShaperCacheKey(FontId fontId,
                       Scalar letterSpacing,
                       TextScript script,
                       bool isRightToLeft,
                       const Character* characters,
                       size_t length);

    bool operator==(const TextShaperCacheKey& other) const;
    bool operator!=(const TextShaperCacheKey& other) const;

    constexpr size_t hash() const {
        return _hash;
    }

private:
    size_t _hash = 0;
};

struct TextShaperCacheValue {
    const ShapedGlyph* glyphs = nullptr;
    size_t length = 0;

    constexpr TextShaperCacheValue() = default;
    constexpr TextShaperCacheValue(const ShapedGlyph* glyphs, size_t length) : glyphs(glyphs), length(length) {}
};

class TextShaperCacheNode : public Valdi::LRUCacheNode<TextShaperCacheKey, TextShaperCacheValue> {
public:
    explicit TextShaperCacheNode(std::pair<TextShaperCacheKey, TextShaperCacheValue>&& keyValue);

    static Ref<TextShaperCacheNode> make(FontId fontId,
                                         Scalar letterSpacing,
                                         TextScript script,
                                         bool isRightToLeft,
                                         const Character* characters,
                                         size_t charactersLength,
                                         const ShapedGlyph* glyphs,
                                         size_t glyphsLength,
                                         size_t hash);
};

/**
 * The TextShaperCache can store shape results from a font id and a set of characters.
 * It uses an LRUCache to store the shape results.
 */
class TextShaperCache {
public:
    explicit TextShaperCache(size_t capacity);
    ~TextShaperCache();

    void clear();

    bool contains(const TextShaperCacheKey& key) const;
    std::optional<TextShaperCacheValue> find(const TextShaperCacheKey& key);
    void insert(const TextShaperCacheKey& key, const ShapedGlyph* glyphs, size_t glyphsLength);

private:
    Valdi::LRUCache<TextShaperCacheKey, TextShaperCacheValue> _cache;
};

} // namespace snap::drawing

namespace std {

template<>
struct hash<snap::drawing::TextShaperCacheKey> {
    std::size_t operator()(const snap::drawing::TextShaperCacheKey& k) const noexcept;
};

} // namespace std
