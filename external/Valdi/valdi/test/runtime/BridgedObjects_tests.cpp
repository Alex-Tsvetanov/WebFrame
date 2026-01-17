#include "valdi/runtime/Utils/BridgedObjectsManager.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "gtest/gtest.h"

using namespace Valdi;

namespace ValdiTest {

TEST(SequenceIDGenerator, generateIncreasingIndexes) {
    SequenceIDGenerator generator;

    ASSERT_EQ(static_cast<uint32_t>(0), generator.newId().getIndex());
    ASSERT_EQ(static_cast<uint32_t>(1), generator.newId().getIndex());
    ASSERT_EQ(static_cast<uint32_t>(2), generator.newId().getIndex());
}

TEST(SequenceIDGenerator, canReuseIndexes) {
    SequenceIDGenerator generator;

    auto id1 = generator.newId();
    ASSERT_EQ(static_cast<uint32_t>(0), id1.getIndex());

    auto id2 = generator.newId();
    ASSERT_EQ(static_cast<uint32_t>(1), id2.getIndex());

    generator.releaseId(id2);

    auto id3 = generator.newId();

    // We should have index 1 because it was released
    ASSERT_EQ(static_cast<uint32_t>(1), id3.getIndex());

    // The actual ids should still be different.
    ASSERT_NE(id2.getId(), id3.getId());

    generator.releaseId(id3);
    generator.releaseId(id1);

    auto id4 = generator.newId();
    ASSERT_EQ(static_cast<uint32_t>(0), id4.getIndex());
}

TEST(SequenceIDGenerator, canReset) {
    SequenceIDGenerator generator;

    ASSERT_EQ(static_cast<uint32_t>(0), generator.newId().getIndex());
    ASSERT_EQ(static_cast<uint32_t>(1), generator.newId().getIndex());
    ASSERT_EQ(static_cast<uint32_t>(2), generator.newId().getIndex());

    generator.reset();

    ASSERT_EQ(static_cast<uint32_t>(0), generator.newId().getIndex());
}

struct BridgeTest {};

Shared<BridgeTest> makeBridgeObject() {
    return Valdi::makeShared<BridgeTest>();
}

using TestBridgedObjectsManager = BridgedObjectsManager<BridgeTest>;

TEST(BridgedObjectsManager, canStoreAndRetrieveObject) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();
    auto obj2 = makeBridgeObject();
    auto obj3 = makeBridgeObject();

    auto id1 = manager.storeObject(obj1);
    auto id2 = manager.storeObject(obj2);
    auto id3 = manager.storeObject(obj3);

    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    ASSERT_TRUE(id3.has_value());

    ASSERT_TRUE(manager.getObject(*id1) != nullptr);
    ASSERT_TRUE(manager.getObject(*id2) != nullptr);
    ASSERT_TRUE(manager.getObject(*id3) != nullptr);

    ASSERT_EQ(obj1, *manager.getObject(*id1));
    ASSERT_EQ(obj2, *manager.getObject(*id2));
    ASSERT_EQ(obj3, *manager.getObject(*id3));
}

TEST(BridgedObjectsManager, canReleaseObject) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();
    ASSERT_EQ(1, obj1.use_count());
    auto id = manager.storeObject(obj1);
    ASSERT_TRUE(id.has_value());
    ASSERT_EQ(2, obj1.use_count());
    manager.releaseObject(*id);
    ASSERT_EQ(1, obj1.use_count());
}

TEST(BridgedObjectsManager, canReleaseAllObjects) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();
    auto obj2 = makeBridgeObject();
    auto obj3 = makeBridgeObject();

    manager.storeObject(obj1);
    manager.storeObject(obj2);
    manager.storeObject(obj3);

    ASSERT_EQ(2, obj1.use_count());
    ASSERT_EQ(2, obj2.use_count());
    ASSERT_EQ(2, obj3.use_count());

    manager.releaseAllObjectsAndFreeze();

    ASSERT_EQ(1, obj1.use_count());
    ASSERT_EQ(1, obj2.use_count());
    ASSERT_EQ(1, obj3.use_count());
}

TEST(BridgedObjectsManager, canFreeze) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();

    manager.storeObject(obj1);

    manager.releaseAllObjectsAndFreeze();

    auto obj2 = makeBridgeObject();

    ASSERT_FALSE(manager.storeObject(obj2).has_value());
}

TEST(BridgedObjectsManager, canRetainByRefCount) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();
    auto id = manager.storeObject(obj1);

    ASSERT_TRUE(id.has_value());
    ASSERT_EQ(2, obj1.use_count());

    manager.retainObject(*id);
    manager.releaseObject(*id);

    // because we retained the release should have removed the object
    ASSERT_EQ(2, obj1.use_count());

    manager.releaseObject(*id);

    // Now it should since it reached 0
    ASSERT_EQ(1, obj1.use_count());
}

TEST(BridgedObjectsManager, releaseAllObjectsKeepRetainedObjects) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();
    auto obj2 = makeBridgeObject();
    auto obj3 = makeBridgeObject();

    manager.storeObject(obj1);
    auto id = manager.storeObject(obj2);
    ASSERT_TRUE(id.has_value());
    manager.storeObject(obj3);

    manager.retainObject(*id);

    ASSERT_EQ(2, obj1.use_count());
    ASSERT_EQ(2, obj2.use_count());
    ASSERT_EQ(2, obj3.use_count());

    manager.releaseAllObjectsAndFreeze();

    ASSERT_EQ(1, obj1.use_count());
    // obj2 was retained so it should be kept.
    ASSERT_EQ(2, obj2.use_count());
    ASSERT_EQ(1, obj3.use_count());

    manager.releaseObject(*id);

    // Should now be destroyed
    ASSERT_EQ(1, obj2.use_count());
}

TEST(BridgedObjectsManager, canForcefullyRemoveObject) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();

    auto id = manager.storeObject(obj1);
    ASSERT_TRUE(id.has_value());

    manager.retainObject(*id);
    manager.retainObject(*id);
    manager.retainObject(*id);
    manager.retainObject(*id);

    ASSERT_EQ(2, obj1.use_count());

    manager.removeObject(*id);

    ASSERT_EQ(1, obj1.use_count());
}

TEST(BridgedObjectsManager, throwOnNonExistingObject) {
    BridgedObjectsManager<Shared<BridgeTest>> manager;

    auto obj1 = makeBridgeObject();
    auto id1 = manager.storeObject(obj1);
    ASSERT_TRUE(id1.has_value());

    ASSERT_TRUE(manager.getObject(42) == nullptr);

    ASSERT_TRUE(manager.getObject(*id1) != nullptr);
    ASSERT_EQ(obj1, *manager.getObject(*id1));

    manager.removeObject(*id1);
    ASSERT_FALSE(manager.getObject(*id1) != nullptr);
}

} // namespace ValdiTest
