#include "valdi_core/cpp/Utils/LRUCache.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(LRUCache, canInsertAndGet) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);

    ASSERT_EQ(static_cast<size_t>(3), cache.size());

    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyD")));

    auto it = cache.find(STRING_LITERAL("KeyA"));

    ASSERT_TRUE(it != cache.end());
    ASSERT_EQ(1, it->value());

    it = cache.find(STRING_LITERAL("KeyB"));

    ASSERT_TRUE(it != cache.end());
    ASSERT_EQ(2, it->value());

    it = cache.find(STRING_LITERAL("KeyC"));

    ASSERT_TRUE(it != cache.end());
    ASSERT_EQ(3, it->value());

    it = cache.find(STRING_LITERAL("KeyD"));

    ASSERT_TRUE(it == cache.end());
}

TEST(LRUCache, canRemove) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);

    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyA")));

    ASSERT_TRUE(cache.remove(STRING_LITERAL("KeyA")));

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));

    auto it = cache.find(STRING_LITERAL("KeyA"));
    ASSERT_TRUE(it == cache.end());

    ASSERT_EQ(static_cast<size_t>(0), cache.size());
    ASSERT_TRUE(cache.empty());
}

TEST(LRUCache, canClear) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);

    ASSERT_EQ(static_cast<size_t>(3), cache.size());
    ASSERT_FALSE(cache.empty());

    cache.clear();

    ASSERT_EQ(static_cast<size_t>(0), cache.size());
    ASSERT_TRUE(cache.empty());

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyC")));
}

TEST(LRUCache, canTrimCapacity) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);
    cache.insert(STRING_LITERAL("KeyD"), 4);

    ASSERT_EQ(static_cast<size_t>(4), cache.size());

    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyD")));

    cache.setCapacity(3);

    ASSERT_EQ(static_cast<size_t>(3), cache.size());

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyD")));

    cache.setCapacity(2);

    ASSERT_EQ(static_cast<size_t>(2), cache.size());

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyD")));

    cache.setCapacity(1);

    ASSERT_EQ(static_cast<size_t>(1), cache.size());

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyD")));

    cache.setCapacity(0);

    ASSERT_EQ(static_cast<size_t>(0), cache.size());

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyD")));
}

TEST(LRUCache, trimsOnInsertionWhenReachingCapacity) {
    LRUCache<StringBox, int> cache(3);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);

    ASSERT_EQ(static_cast<size_t>(3), cache.size());

    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyD")));

    cache.insert(STRING_LITERAL("KeyD"), 3);

    ASSERT_EQ(static_cast<size_t>(3), cache.size());

    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyD")));

    cache.insert(STRING_LITERAL("KeyA"), 3);
    ASSERT_EQ(static_cast<size_t>(3), cache.size());

    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyA")));
    ASSERT_FALSE(cache.contains(STRING_LITERAL("KeyB")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyC")));
    ASSERT_TRUE(cache.contains(STRING_LITERAL("KeyD")));
}

TEST(LRUCache, canIterate) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);

    auto it = cache.begin();

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyC"), it->key());
    ASSERT_EQ(3, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyB"), it->key());
    ASSERT_EQ(2, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyA"), it->key());
    ASSERT_EQ(1, it->value());

    it++;

    ASSERT_TRUE(it == cache.end());
}

TEST(LRUCache, reordersOnInsert) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);
    cache.insert(STRING_LITERAL("KeyB"), 2);

    auto it = cache.begin();

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyB"), it->key());
    ASSERT_EQ(2, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyC"), it->key());
    ASSERT_EQ(3, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyA"), it->key());
    ASSERT_EQ(1, it->value());

    it++;

    ASSERT_TRUE(it == cache.end());
}

TEST(LRUCache, reordersOnFind) {
    LRUCache<StringBox, int> cache(16);

    cache.insert(STRING_LITERAL("KeyA"), 1);
    cache.insert(STRING_LITERAL("KeyB"), 2);
    cache.insert(STRING_LITERAL("KeyC"), 3);

    cache.find(STRING_LITERAL("KeyA"));

    auto it = cache.begin();

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyA"), it->key());
    ASSERT_EQ(1, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyC"), it->key());
    ASSERT_EQ(3, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyB"), it->key());
    ASSERT_EQ(2, it->value());

    it++;

    ASSERT_TRUE(it == cache.end());

    // Find with KeyC, to move it at the beginning of the list
    cache.find(STRING_LITERAL("KeyC"));

    it = cache.begin();

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyC"), it->key());
    ASSERT_EQ(3, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyA"), it->key());
    ASSERT_EQ(1, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyB"), it->key());
    ASSERT_EQ(2, it->value());

    it++;

    ASSERT_TRUE(it == cache.end());

    // Find with KeyB
    cache.find(STRING_LITERAL("KeyB"));

    it = cache.begin();

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyB"), it->key());
    ASSERT_EQ(2, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyC"), it->key());
    ASSERT_EQ(3, it->value());

    it++;

    ASSERT_FALSE(it == cache.end());

    ASSERT_EQ(STRING_LITERAL("KeyA"), it->key());
    ASSERT_EQ(1, it->value());

    it++;

    ASSERT_TRUE(it == cache.end());
}

} // namespace ValdiTest
