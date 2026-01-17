//
//  ObjectPool_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 9/30/19.
//

#include "valdi_core/cpp/Utils/ObjectPool.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

struct Object {
    Object() = default;

    Value value;
};

void cleanUpObject(Shared<Object>& object) {
    object->value = Value();
}

using ReusableObject = ObjectPoolEntry<Shared<Object>, void (*)(Shared<Object>&)>;

class Pool {
public:
    ReusableObject create() {
        return _innerPool.getOrCreate(
            [&]() {
                _createdObjectsCount++;
                return Valdi::makeShared<Object>();
            },
            &cleanUpObject);
    }

    int getCreatedObjectsCount() const {
        return _createdObjectsCount;
    }

private:
    ObjectPoolInner<Shared<Object>> _innerPool;
    int _createdObjectsCount = 0;
};

TEST(ObjectPool, canCreateAndReuse) {
    auto pool = Pool();
    ASSERT_EQ(0, pool.getCreatedObjectsCount());

    Object* firstCreatedObject = nullptr;
    Object* secondCreatedObject = nullptr;

    {
        auto object = pool.create();
        ASSERT_EQ(1, pool.getCreatedObjectsCount());
        firstCreatedObject = object->get();
        auto object2 = pool.create();
        ASSERT_EQ(2, pool.getCreatedObjectsCount());
        secondCreatedObject = object2->get();
    }

    // Those objects should be reused
    auto object = pool.create();
    ASSERT_EQ(2, pool.getCreatedObjectsCount());
    ASSERT_TRUE(object.get() != nullptr);
    ASSERT_EQ(object->get(), secondCreatedObject);

    auto object2 = pool.create();
    ASSERT_EQ(2, pool.getCreatedObjectsCount());
    ASSERT_TRUE(object2.get() != nullptr);
    ASSERT_EQ(object2->get(), firstCreatedObject);
}

TEST(ObjectPool, cleanupOnRecycle) {
    auto pool = Pool();

    {
        auto object = pool.create();
        object->get()->value = Value(42);
        ASSERT_EQ(Value(42), object->get()->value);
    }

    ASSERT_EQ(1, pool.getCreatedObjectsCount());
    auto object = pool.create();
    ASSERT_EQ(1, pool.getCreatedObjectsCount());

    ASSERT_NE(Value(42), object->get()->value);
    ASSERT_EQ(Value(), object->get()->value);
}

TEST(ObjectPool, canMoveObjects) {
    auto pool = Pool();

    {
        auto object = pool.create();
        object->get()->value = Value(42);

        auto object2 = pool.create();
        object2->get()->value = Value(84);

        ASSERT_EQ(Value(42), object->get()->value);
        ASSERT_EQ(Value(84), object2->get()->value);

        object2 = std::move(object);

        ASSERT_TRUE(object->get() == nullptr);
        ASSERT_EQ(Value(42), object2->get()->value);

        ASSERT_EQ(2, pool.getCreatedObjectsCount());

        // Since we moved one object, we should be able to create one without
        // creating a new instance

        auto object3 = pool.create();
        ASSERT_EQ(2, pool.getCreatedObjectsCount());

        ASSERT_EQ(Value(), object3->get()->value);
    }

    // We are out of scope, we should have released the two allocated objects

    auto object = pool.create();
    auto object2 = pool.create();
    ASSERT_EQ(2, pool.getCreatedObjectsCount());

    ASSERT_EQ(Value(), object->get()->value);
    ASSERT_EQ(Value(), object2->get()->value);
}

} // namespace ValdiTest
