#include "valdi_core/cpp/Utils/ReferenceTable.hpp"

#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"

#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(ReferenceTable, canMakeRef) {
    ReferenceTable table;

    {
        auto write = table.writeAccess();
        auto id = write.makeRef("MyRef").id;

        ASSERT_EQ(static_cast<uint32_t>(1), id.getSalt());
        ASSERT_EQ(static_cast<uint32_t>(0), id.getIndex());

        auto id2 = write.makeRef("MyRef").id;

        ASSERT_EQ(static_cast<uint32_t>(2), id2.getSalt());
        ASSERT_EQ(static_cast<uint32_t>(1), id2.getIndex());

        auto id3 = write.makeRef("MyOtherRef").id;

        ASSERT_EQ(static_cast<uint32_t>(3), id3.getSalt());
        ASSERT_EQ(static_cast<uint32_t>(2), id3.getIndex());

        ASSERT_EQ(static_cast<size_t>(1), write.getEntry(id).retainCount);
        ASSERT_EQ(static_cast<size_t>(1), write.getEntry(id2).retainCount);
        ASSERT_EQ(static_cast<size_t>(1), write.getEntry(id3).retainCount);
    }

    auto allReferences = table.readAccess().getAll();
    ASSERT_EQ(static_cast<size_t>(3), allReferences.size());

    ASSERT_EQ(ReferenceTableEntry(SequenceID(0, 1), 1, "MyRef"), allReferences[0]);
    ASSERT_EQ(ReferenceTableEntry(SequenceID(1, 2), 1, "MyRef"), allReferences[1]);
    ASSERT_EQ(ReferenceTableEntry(SequenceID(2, 3), 1, "MyOtherRef"), allReferences[2]);
}

TEST(ReferenceTable, canRetainRef) {
    ReferenceTable table;

    auto write = table.writeAccess();
    auto id = write.makeRef("MyRef").id;
    ASSERT_EQ(static_cast<size_t>(1), write.getEntry(id).retainCount);

    write.retainRef(id);

    ASSERT_EQ(static_cast<size_t>(2), write.getEntry(id).retainCount);

    write.retainRef(id);

    ASSERT_EQ(static_cast<size_t>(3), write.getEntry(id).retainCount);
}

TEST(ReferenceTable, canReleaseRef) {
    ReferenceTable table;

    SequenceID id;
    {
        auto write = table.writeAccess();
        id = write.makeRef("MyRef").id;
        write.retainRef(id);
    }

    auto allReferences = table.readAccess().getAll();
    ASSERT_EQ(static_cast<size_t>(1), allReferences.size());

    ASSERT_EQ(ReferenceTableEntry(SequenceID(0, 1), 2, "MyRef"), allReferences[0]);

    table.writeAccess().releaseRef(id);

    allReferences = table.readAccess().getAll();
    ASSERT_EQ(static_cast<size_t>(1), allReferences.size());
    ASSERT_EQ(ReferenceTableEntry(SequenceID(0, 1), 1, "MyRef"), allReferences[0]);

    table.writeAccess().releaseRef(id);

    allReferences = table.readAccess().getAll();
    ASSERT_EQ(static_cast<size_t>(0), allReferences.size());
}

TEST(ReferenceTable, canReuseIndexes) {
    ReferenceTable table;

    {
        auto write = table.writeAccess();

        auto id = write.makeRef("MyRef").id;
        auto id2 = write.makeRef("MyRef2").id;

        ASSERT_EQ(SequenceID(0, 1), id);
        ASSERT_EQ(SequenceID(1, 2), id2);

        write.releaseRef(id);
        id = write.makeRef("MyRef3").id;

        ASSERT_EQ(SequenceID(0, 3), id);

        auto id3 = write.makeRef("MyRef4").id;

        ASSERT_EQ(SequenceID(2, 4), id3);

        write.releaseRef(id3);
        id3 = write.makeRef("MyRef5").id;

        ASSERT_EQ(SequenceID(2, 5), id3);
    }

    auto allReferences = table.readAccess().getAll();
    ASSERT_EQ(static_cast<size_t>(3), allReferences.size());

    ASSERT_EQ(ReferenceTableEntry(SequenceID(0, 3), 1, "MyRef3"), allReferences[0]);
    ASSERT_EQ(ReferenceTableEntry(SequenceID(1, 2), 1, "MyRef2"), allReferences[1]);
    ASSERT_EQ(ReferenceTableEntry(SequenceID(2, 5), 1, "MyRef5"), allReferences[2]);
}

TEST(ReferenceTable, canDumpTableStats) {
    ReferenceTable table;

    {
        auto write = table.writeAccess();
        write.makeRef("MyRef");
        write.makeRef("MyRef2");
        write.makeRef("MyRef");
        write.makeRef("MyRef");
        auto id5 = write.makeRef("MyRef").id;

        write.releaseRef(id5);
    }

    auto stats = table.dumpStats();

    ASSERT_EQ(static_cast<size_t>(5), stats.tableSize);
    ASSERT_EQ(static_cast<size_t>(4), stats.activeReferencesCount);

    std::vector<std::pair<const char*, size_t>> expectedActiveReferencesByTag = {
        std::make_pair<const char*, size_t>("MyRef", 3), std::make_pair<const char*, size_t>("MyRef2", 1)};

    ASSERT_EQ(expectedActiveReferencesByTag, stats.activeReferencesByTag);
}

TEST(ReferenceTable, isThreadSafe) {
    ReferenceTable table;

    std::vector<Ref<DispatchQueue>> writeQueues;
    for (size_t i = 0; i < 4; i++) {
        writeQueues.emplace_back(DispatchQueue::createThreaded(
            STRING_LITERAL("com.snap.valdi.ReferenceTable.writeQueue"), ThreadQoSClassHigh));
    }

    size_t iterationsCount = 100;

    const char* ref1 = "Ref1";
    const char* ref2 = "Ref2";
    const char* ref3 = "Ref3";
    SequenceID id1;
    SequenceID id2;
    SequenceID id3;
    bool tick = false;
    {
        auto write = table.writeAccess();
        id1 = write.makeRef(ref1).id;
        id2 = write.makeRef(ref2).id;
        id3 = write.makeRef(ref3).id;
    }

    // Use a group to get the read and write tasks to start essentially at the same time
    AsyncGroup group;
    std::atomic_int completedCount = 0;

    for ([[maybe_unused]] const auto& queue : writeQueues) {
        group.enter();
    }

    for (const auto& queue : writeQueues) {
        queue->async([&]() {
            group.leave();
            group.blockingWait();

            for (size_t i = 0; i < iterationsCount; i++) {
                auto write = table.writeAccess();
                tick = !tick;

                if (tick) {
                    write.retainRef(id1);
                    write.releaseRef(id2);
                    id2 = write.makeRef(ref2).id;
                    write.retainRef(id3);
                    write.retainRef(id3);
                } else {
                    write.releaseRef(id1);
                    write.releaseRef(id1);
                    write.releaseRef(id2);
                    id1 = write.makeRef(ref1).id;
                    id2 = write.makeRef(ref2).id;
                    write.releaseRef(id3);
                    write.releaseRef(id3);
                }
            }

            ++completedCount;
        });
    }

    auto expectedCompletedCount = static_cast<int>(writeQueues.size());
    std::vector<ReferenceTableEntry> entries;

    while (expectedCompletedCount != completedCount) {
        entries.clear();
        bool isTick;
        {
            auto read = table.readAccess();
            isTick = tick;
            read.getAll(entries);
        }

        ASSERT_EQ(static_cast<size_t>(3), entries.size());

        for (const auto& entry : entries) {
            if (entry.tag == ref1) {
                if (isTick) {
                    ASSERT_EQ(static_cast<size_t>(2), entry.retainCount);
                } else {
                    ASSERT_EQ(static_cast<size_t>(1), entry.retainCount);
                }
            } else if (entry.tag == ref2) {
                ASSERT_EQ(static_cast<size_t>(1), entry.retainCount);
            } else if (entry.tag == ref3) {
                if (isTick) {
                    ASSERT_EQ(static_cast<size_t>(3), entry.retainCount);
                } else {
                    ASSERT_EQ(static_cast<size_t>(1), entry.retainCount);
                }
            } else {
                ASSERT_EQ(false, true);
            }
        }
    }

    for (const auto& queue : writeQueues) {
        queue->fullTeardown();
    }
}

} // namespace ValdiTest
