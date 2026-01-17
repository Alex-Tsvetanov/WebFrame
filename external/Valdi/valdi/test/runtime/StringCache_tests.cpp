#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

static bool containsStringInCache(const StringCache& cache, std::string_view str) {
    for (const auto& strInCache : cache.all()) {
        if (strInCache.toStringView() == str) {
            return true;
        }
    }

    return false;
}

TEST(StringCache, canMakeStrings) {
    auto& cache = StringCache::getGlobal();

    {
        auto str = cache.makeStringFromLiteral("hello is a test string");

        ASSERT_EQ(static_cast<size_t>(22), str.length());
        ASSERT_EQ("hello is a test string", str.toStringView());

        ASSERT_TRUE(containsStringInCache(cache, "hello is a test string"));
    }

    ASSERT_FALSE(containsStringInCache(cache, "hello is a test string"));
}

TEST(StringCache, canInternStrings) {
    auto& cache = StringCache::getGlobal();

    auto str1 = cache.makeStringFromLiteral("hello");
    auto str2 = cache.makeStringFromLiteral("hello");
    auto str3 = cache.makeStringFromLiteral("hello!");

    ASSERT_EQ(str1, str2);
    ASSERT_EQ("hello", str1.toStringView());
    ASSERT_EQ(str1.getInternedString(), str1.getInternedString());

    ASSERT_NE(str1, str3);
    ASSERT_NE(str1.getInternedString(), str3.getInternedString());
}

TEST(StringCache, canDeallocConcurrently) {
    auto& cache = StringCache::getGlobal();

    auto queue1 = DispatchQueue::create(STRING_LITERAL("Thread1"), ThreadQoSClassMax);
    auto queue2 = DispatchQueue::create(STRING_LITERAL("Thread2"), ThreadQoSClassMax);
    auto queue3 = DispatchQueue::create(STRING_LITERAL("Thread3"), ThreadQoSClassMax);

    for (size_t i = 0; i < 10; i++) {
        auto str = cache.makeStringFromLiteral("StringToTest");
        StringBox str2;
        StringBox str3;

        auto lock = cache.lock();

        queue1->async([&]() {
            // In Thread1, we release the string
            str = StringBox();
        });

        queue2->async([&]() { str2 = cache.makeStringFromLiteral("StringToTest"); });

        queue2->async([&]() { str3 = cache.makeStringFromLiteral("StringToTest"); });

        lock.unlock();
        queue1->sync([]() {});
        queue2->sync([]() {});
        queue3->sync([]() {});

        ASSERT_EQ(str2, str3);
    }
}

} // namespace ValdiTest
