#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(ValdiArchive, supportsPadding) {
    auto key = STRING_LITERAL("1");
    auto data = STRING_LITERAL("!?");

    ValdiArchiveBuilder builder;
    builder.addEntry(ValdiArchiveEntry(key, data));

    auto archiveBytes = builder.build();

    // Data layout with padding support:
    // 4 bytes magic | 4 bytes data length | 4 bytes file name length | 1 byte file name | 3 bytes padding | 4 bytes
    // data length | 2 bytes data | 2 bytes padding
    ASSERT_EQ(
        static_cast<size_t>(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + 1 + 3 + sizeof(uint32_t) + 2 + 2),
        archiveBytes->size());

    ValdiArchive archive(archiveBytes->begin(), archiveBytes->end());

    auto result = archive.getEntries();

    ASSERT_TRUE(result) << result.description();

    const auto& entries = result.value();

    ASSERT_EQ(static_cast<size_t>(1), entries.size());

    const auto& entry = entries[0];
    auto dataStr = std::string_view(reinterpret_cast<const char*>(entry.data), entry.dataLength);

    ASSERT_EQ(STRING_LITERAL("1"), entry.filePath);
    ASSERT_EQ("!?", dataStr);
}

} // namespace ValdiTest
