#include "valdi/runtime/Utils/RefCountableAutoreleasePool.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "gtest/gtest.h"

using namespace Valdi;

namespace ValdiTest {

struct RefCountableObject : public SimpleRefCountable {};

struct CallbackOnDestruct : public SimpleRefCountable {
    Valdi::DispatchFunction callback;

    CallbackOnDestruct(Valdi::DispatchFunction&& callback) : callback(std::move(callback)) {}

    ~CallbackOnDestruct() {
        callback();
    }
};

TEST(RefCountableAutoreleasePool, releaseOnDestruct) {
    auto obj = makeShared<RefCountableObject>();

    auto* ptr = Valdi::unsafeBridgeRetain(obj.get());
    ASSERT_EQ(static_cast<long>(2), obj.use_count());
    {
        RefCountableAutoreleasePool pool;
        RefCountableAutoreleasePool::release(ptr);
        ASSERT_EQ(static_cast<long>(2), obj.use_count());
    }
    ASSERT_EQ(static_cast<long>(1), obj.use_count());
}

TEST(RefCountableAutoreleasePool, releaseImmediatelyWhenNoPoolIsCurrent) {
    auto obj = makeShared<RefCountableObject>();
    auto* ptr = Valdi::unsafeBridgeRetain(obj.get());
    ASSERT_EQ(static_cast<long>(2), obj.use_count());
    RefCountableAutoreleasePool::release(ptr);
    ASSERT_EQ(static_cast<long>(1), obj.use_count());
}

TEST(RefCountableAutoreleasePool, supportsNestedPools) {
    auto obj1 = makeShared<RefCountableObject>();
    auto obj2 = makeShared<RefCountableObject>();
    auto obj3 = makeShared<RefCountableObject>();

    {
        RefCountableAutoreleasePool outerPool;
        RefCountableAutoreleasePool::release(Valdi::unsafeBridgeRetain(obj1.get()));
        {
            RefCountableAutoreleasePool innerPool;
            RefCountableAutoreleasePool::release(Valdi::unsafeBridgeRetain(obj1.get()));
            RefCountableAutoreleasePool::release(Valdi::unsafeBridgeRetain(obj2.get()));
            ASSERT_EQ(static_cast<long>(3), obj1.use_count());
            ASSERT_EQ(static_cast<long>(2), obj2.use_count());
        }
        ASSERT_EQ(static_cast<long>(2), obj1.use_count());
        ASSERT_EQ(static_cast<long>(1), obj2.use_count());

        RefCountableAutoreleasePool::release(Valdi::unsafeBridgeRetain(obj3.get()));
        ASSERT_EQ(static_cast<long>(2), obj3.use_count());
    }

    ASSERT_EQ(static_cast<long>(1), obj1.use_count());
    ASSERT_EQ(static_cast<long>(1), obj2.use_count());
    ASSERT_EQ(static_cast<long>(1), obj3.use_count());
}

TEST(RefCountableAutoreleasePool, canReleaseObjectWhileDestructing) {
    auto inner = makeShared<RefCountableObject>();

    size_t callbackCallCount = 0;
    auto* innerPtr = Valdi::unsafeBridgeRetain(inner.get());
    auto callbackOnDestruct = makeShared<CallbackOnDestruct>([&]() {
        callbackCallCount++;
        RefCountableAutoreleasePool::release(innerPtr);
    });

    auto* callbackOnDestructPtr = Valdi::unsafeBridgeRetain(callbackOnDestruct.get());

    ASSERT_EQ(static_cast<long>(2), inner.use_count());
    ASSERT_EQ(static_cast<long>(2), callbackOnDestruct.use_count());
    callbackOnDestruct = nullptr;

    ASSERT_EQ(static_cast<size_t>(0), callbackCallCount);
    ASSERT_EQ(static_cast<long>(2), inner.use_count());

    {
        RefCountableAutoreleasePool pool;
        RefCountableAutoreleasePool::release(callbackOnDestructPtr);
        ASSERT_EQ(static_cast<long>(2), inner.use_count());
    }

    ASSERT_EQ(static_cast<size_t>(1), callbackCallCount);
    ASSERT_EQ(static_cast<long>(1), inner.use_count());
}

TEST(RefCountableAutoreleasePool, canCreatePoolWhileDestructing) {
    auto inner = makeShared<RefCountableObject>();
    auto* innerPtr = Valdi::unsafeBridgeRetain(inner.get());

    auto callbackOnDestruct = makeShared<CallbackOnDestruct>([&]() {
        ASSERT_EQ(static_cast<long>(2), inner.use_count());
        {
            RefCountableAutoreleasePool innerPool;
            RefCountableAutoreleasePool::release(innerPtr);
        }
        ASSERT_EQ(static_cast<long>(1), inner.use_count());
    });

    auto* callbackOnDestructPtr = Valdi::unsafeBridgeRetain(callbackOnDestruct.get());
    callbackOnDestruct = nullptr;

    ASSERT_EQ(static_cast<long>(2), inner.use_count());

    {
        RefCountableAutoreleasePool pool;
        RefCountableAutoreleasePool::release(callbackOnDestructPtr);
        ASSERT_EQ(static_cast<long>(2), inner.use_count());
    }

    ASSERT_EQ(static_cast<long>(1), inner.use_count());
}

TEST(RefCountableAutoreleasePool, limitsEntriesGrow) {
    std::vector<Ref<RefCountableObject>> objects;
    for (size_t i = 0; i < 7; i++) {
        objects.emplace_back(makeShared<RefCountableObject>());
    }

    size_t poolSizeBeforeAddingItem = 0;
    size_t poolSizeAfterAddingItem = 0;

    auto callbackOnDestruct = makeShared<CallbackOnDestruct>([&]() {
        auto objectToAdd = makeShared<RefCountableObject>();
        auto* pool = RefCountableAutoreleasePool::current();
        ASSERT_TRUE(pool != nullptr);
        poolSizeBeforeAddingItem = pool->backingArraySize();
        RefCountableAutoreleasePool::release(Valdi::unsafeBridgeRetain(objectToAdd.get()));
        poolSizeAfterAddingItem = pool->backingArraySize();

        objects.emplace_back(objectToAdd);
        ASSERT_EQ(static_cast<long>(3), objectToAdd.use_count());
    });

    auto* callbackOnDestructPtr = Valdi::unsafeBridgeRetain(callbackOnDestruct.get());
    callbackOnDestruct = nullptr;

    {
        RefCountableAutoreleasePool pool;
        for (const auto& obj : objects) {
            RefCountableAutoreleasePool::release(Valdi::unsafeBridgeRetain(obj.get()));
        }
        RefCountableAutoreleasePool::release(callbackOnDestructPtr);
    }

    ASSERT_EQ(static_cast<size_t>(8), objects.size());
    for (const auto& obj : objects) {
        ASSERT_EQ(static_cast<long>(1), obj.use_count());
    }

    ASSERT_EQ(static_cast<size_t>(8), poolSizeBeforeAddingItem);
    // Pool should have prevented to grow its backing array since it had free spots at the beginning
    ASSERT_EQ(static_cast<size_t>(1), poolSizeAfterAddingItem);
}

} // namespace ValdiTest
