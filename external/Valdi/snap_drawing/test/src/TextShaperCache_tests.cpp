#include <gtest/gtest.h>

#include "snap_drawing/cpp/Text/TextShaperCache.hpp"
#include "snap_drawing/cpp/Utils/UTFUtils.hpp"

namespace snap::drawing {

static std::vector<ShapedGlyph> toGlyphVec(const ShapedGlyph* glyphs, size_t length) {
    std::vector<ShapedGlyph> out;
    out.reserve(length);

    for (size_t i = 0; i < length; i++) {
        out.emplace_back() = glyphs[i];
    }

    return out;
}

static std::vector<ShapedGlyph> generateGlyphVec(const Character* characters, size_t length) {
    std::vector<ShapedGlyph> out;
    out.resize(length);

    auto* data = out.data();
    for (size_t i = 0; i < length; i++) {
        auto& glyph = data[i];
        glyph.offsetX = static_cast<Scalar>(i);
        glyph.offsetY = -static_cast<Scalar>(i);
        glyph.advanceX = static_cast<Scalar>(i) * 2.0f;
        glyph.glyphID = static_cast<uint32_t>(i);

        glyph.setCharacter(characters[i], false);
    }

    return out;
}

static TextShaperCacheKey makeCacheKey(FontId fontId, const std::vector<Character>& characters) {
    return TextShaperCacheKey(fontId, 0.0f, TextScript::invalid(), false, characters.data(), characters.size());
}

TEST(TextShaperCache, canInsertAndGet) {
    TextShaperCache cache(8);

    auto characters = utf8ToUnicode("Hello World what is going on!");

    auto result = cache.find(makeCacheKey(1, characters));

    ASSERT_FALSE(result);

    auto shapedGlyphs = generateGlyphVec(characters.data(), characters.size());

    cache.insert(makeCacheKey(1, characters), shapedGlyphs.data(), shapedGlyphs.size());

    result = cache.find(makeCacheKey(1, characters));

    ASSERT_TRUE(result);

    ASSERT_EQ(shapedGlyphs, toGlyphVec(result.value().glyphs, result.value().length));
}

TEST(TextShaperCache, useFontIdAsKey) {
    TextShaperCache cache(8);

    auto characters = utf8ToUnicode("Hello World what is going on!");
    auto shapedGlyphs = generateGlyphVec(characters.data(), characters.size());

    cache.insert(makeCacheKey(1, characters), shapedGlyphs.data(), shapedGlyphs.size());
    cache.insert(makeCacheKey(2, characters), shapedGlyphs.data(), shapedGlyphs.size());
    cache.insert(makeCacheKey(3, characters), shapedGlyphs.data(), shapedGlyphs.size());

    auto result = cache.find(makeCacheKey(1, characters));

    ASSERT_TRUE(result);

    result = cache.find(makeCacheKey(2, characters));

    ASSERT_TRUE(result);

    result = cache.find(makeCacheKey(3, characters));

    ASSERT_TRUE(result);

    result = cache.find(makeCacheKey(4, characters));

    ASSERT_FALSE(result);

    result = cache.find(makeCacheKey(0, characters));

    ASSERT_FALSE(result);
}

TEST(TextShaperCache, useCharactersAsKey) {
    TextShaperCache cache(8);

    auto characters = utf8ToUnicode("Hello World what is going on!");
    auto characters2 = utf8ToUnicode("Hello World what is going on.");
    auto characters3 = utf8ToUnicode("I'm way different.");
    auto characters4 = utf8ToUnicode("I dont exist");

    auto shapedGlyphs = generateGlyphVec(characters.data(), characters.size());
    auto shapedGlyphs2 = generateGlyphVec(characters2.data(), characters2.size());
    auto shapedGlyphs3 = generateGlyphVec(characters3.data(), characters3.size());

    cache.insert(makeCacheKey(1, characters), shapedGlyphs.data(), shapedGlyphs.size());
    cache.insert(makeCacheKey(1, characters2), shapedGlyphs2.data(), shapedGlyphs2.size());
    cache.insert(makeCacheKey(1, characters3), shapedGlyphs3.data(), shapedGlyphs3.size());

    auto result = cache.find(makeCacheKey(1, characters));

    ASSERT_TRUE(result);

    ASSERT_EQ(shapedGlyphs, toGlyphVec(result.value().glyphs, result.value().length));
    ASSERT_NE(shapedGlyphs2, toGlyphVec(result.value().glyphs, result.value().length));
    ASSERT_NE(shapedGlyphs3, toGlyphVec(result.value().glyphs, result.value().length));

    result = cache.find(makeCacheKey(1, characters2));

    ASSERT_TRUE(result);

    ASSERT_NE(shapedGlyphs, toGlyphVec(result.value().glyphs, result.value().length));
    ASSERT_EQ(shapedGlyphs2, toGlyphVec(result.value().glyphs, result.value().length));
    ASSERT_NE(shapedGlyphs3, toGlyphVec(result.value().glyphs, result.value().length));

    result = cache.find(makeCacheKey(1, characters3));

    ASSERT_TRUE(result);

    ASSERT_NE(shapedGlyphs, toGlyphVec(result.value().glyphs, result.value().length));
    ASSERT_NE(shapedGlyphs2, toGlyphVec(result.value().glyphs, result.value().length));
    ASSERT_EQ(shapedGlyphs3, toGlyphVec(result.value().glyphs, result.value().length));

    result = cache.find(makeCacheKey(1, characters4));

    ASSERT_FALSE(result);
}

TEST(TextShaperCache, canClear) {
    TextShaperCache cache(8);

    auto characters = utf8ToUnicode("Hello World what is going on!");
    auto characters2 = utf8ToUnicode("Hello World what is going on.");

    auto shapedGlyphs = generateGlyphVec(characters.data(), characters.size());
    auto shapedGlyphs2 = generateGlyphVec(characters2.data(), characters2.size());

    cache.insert(makeCacheKey(1, characters), shapedGlyphs.data(), shapedGlyphs.size());
    cache.insert(makeCacheKey(1, characters2), shapedGlyphs2.data(), shapedGlyphs2.size());

    cache.clear();

    auto result = cache.find(makeCacheKey(1, characters));

    ASSERT_FALSE(result);

    result = cache.find(makeCacheKey(1, characters2));

    ASSERT_FALSE(result);
}

TEST(TextShaperCache, removesLeastRecentlyUsedWhenFull) {
    TextShaperCache cache(3);

    auto characters = utf8ToUnicode("Hello World what is going on!");
    auto characters2 = utf8ToUnicode("Hello World what is going on.");
    auto characters3 = utf8ToUnicode("I'm way different.");
    auto characters4 = utf8ToUnicode("I dont exist");

    auto shapedGlyphs = generateGlyphVec(characters.data(), characters.size());
    auto shapedGlyphs2 = generateGlyphVec(characters2.data(), characters2.size());
    auto shapedGlyphs3 = generateGlyphVec(characters3.data(), characters3.size());
    auto shapedGlyphs4 = generateGlyphVec(characters4.data(), characters4.size());

    cache.insert(makeCacheKey(1, characters), shapedGlyphs.data(), shapedGlyphs.size());
    cache.insert(makeCacheKey(1, characters2), shapedGlyphs2.data(), shapedGlyphs2.size());
    cache.insert(makeCacheKey(1, characters3), shapedGlyphs3.data(), shapedGlyphs3.size());
    cache.insert(makeCacheKey(1, characters4), shapedGlyphs4.data(), shapedGlyphs4.size());

    ASSERT_FALSE(cache.contains(makeCacheKey(1, characters)));
    ASSERT_TRUE(cache.contains(makeCacheKey(1, characters2)));
    ASSERT_TRUE(cache.contains(makeCacheKey(1, characters3)));
    ASSERT_TRUE(cache.contains(makeCacheKey(1, characters4)));

    // Use characters2 so that it becomes the most recently used, this way the oldest
    // item becomes characters3
    ASSERT_TRUE(cache.find(makeCacheKey(1, characters2)));

    // Insert the first one so that the cache needs to remove an item
    cache.insert(makeCacheKey(1, characters), shapedGlyphs.data(), shapedGlyphs.size());

    ASSERT_TRUE(cache.contains(makeCacheKey(1, characters)));
    ASSERT_TRUE(cache.contains(makeCacheKey(1, characters2)));
    ASSERT_FALSE(cache.contains(makeCacheKey(1, characters3)));
    ASSERT_TRUE(cache.contains(makeCacheKey(1, characters4)));
}

} // namespace snap::drawing
