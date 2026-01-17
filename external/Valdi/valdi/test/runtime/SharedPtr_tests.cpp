//
//  SharedPtr_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/23/19.
//

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

struct Int : public SharedPtrRefCountable {
    int value;

    Int(int value) : value(value) {}
};

struct Container : public SimpleRefCountable {
    Shared<Int> value;
};

struct Outer : public SharedPtrRefCountable {
    Shared<Int> inner;

    Outer() {
        inner = Valdi::makeShared<Int>(0).toShared();
    }
};

TEST(SharedPtr, canBridgeRetainAndRelease) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    ASSERT_EQ(1, object.use_count());
    auto ptr = unsafeRetain(object);
    ASSERT_EQ(2, object.use_count());
    unsafeRelease(ptr);
    ASSERT_EQ(1, object.use_count());
}

TEST(SharedPtr, canAllocateSharedPtrRefCountable) {
    auto outer = Valdi::makeShared<Outer>();

    ASSERT_EQ(1, outer.use_count());

    auto outerShared = outer.toShared();

    ASSERT_EQ(2, outer.use_count());
    ASSERT_EQ(2, outerShared.use_count());

    outer = nullptr;

    ASSERT_EQ(1, outerShared.use_count());
}

TEST(SharedPtr, canDeallocateOnBridgeRelease) {
    auto outer = Valdi::makeShared<Outer>().toShared();
    Weak<Int> inner = outer->inner;

    ASSERT_EQ(1, inner.use_count());
    auto ptr = unsafeRetain(outer);
    outer = nullptr;
    ASSERT_EQ(1, inner.use_count());
    unsafeRelease(ptr);
    ASSERT_EQ(0, inner.use_count());
}

TEST(SharedPtr, canBridge) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    ASSERT_EQ(1, object.use_count());
    auto ptr = unsafeRetain(object);
    ASSERT_EQ(2, object.use_count());

    auto object2 = strongRef(ptr);
    ASSERT_EQ(3, object.use_count());
    ASSERT_EQ(object, object2);

    unsafeRelease(ptr);
    ASSERT_EQ(2, object.use_count());

    object2 = nullptr;

    ASSERT_EQ(1, object.use_count());
}

TEST(SharedPtr, canRetainFromRawPtr) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    ASSERT_EQ(1, object.use_count());
    auto ptr = unsafeRetain(object);
    ASSERT_EQ(2, object.use_count());
    unsafeRetain(ptr);
    ASSERT_EQ(3, object.use_count());
    unsafeRelease(ptr);
    ASSERT_EQ(2, object.use_count());
    unsafeRelease(ptr);
    ASSERT_EQ(1, object.use_count());
}

TEST(SharedPtr, canMove) {
    auto object = Valdi::makeShared<Int>(0).toShared();
    auto copy = object;

    ASSERT_EQ(2, object.use_count());

    auto rawPtr = unsafeSharedMove(std::move(copy));

    ASSERT_EQ(2, object.use_count());
    ASSERT_TRUE(copy == nullptr);
    ASSERT_EQ(object.get(), rawPtr);

    copy = nullptr;
    ASSERT_EQ(2, object.use_count());

    unsafeRelease(rawPtr);
    ASSERT_EQ(1, object.use_count());
}

TEST(Ref, canRetainAndRelease) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    Ref<Int> ptr(object.get());

    ASSERT_EQ(object.get(), ptr.get());
    ASSERT_EQ(2, object.use_count());

    ptr = nullptr;

    ASSERT_EQ(nullptr, ptr.get());
    ASSERT_EQ(1, object.use_count());
}

TEST(Ref, retainAndReleaseOnCopy) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    Ref<Int> ptr(object.get());

    ASSERT_EQ(2, object.use_count());

    Ref<Int> copy1(ptr);
    ASSERT_EQ(object.get(), copy1.get());
    ASSERT_EQ(3, object.use_count());

    auto copy2 = copy1;
    ASSERT_EQ(object.get(), copy2.get());
    ASSERT_EQ(4, object.use_count());

    copy2 = nullptr;
    ASSERT_EQ(nullptr, copy2.get());
    ASSERT_EQ(3, object.use_count());

    copy1 = nullptr;
    ASSERT_EQ(nullptr, copy1.get());
    ASSERT_EQ(2, object.use_count());

    ptr = nullptr;
    ASSERT_EQ(nullptr, ptr.get());
    ASSERT_EQ(1, object.use_count());
}

TEST(Ref, canMove) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    Ref<Int> ptr(object.get());

    ASSERT_EQ(2, object.use_count());

    Ref<Int> move1(std::move(ptr));
    ASSERT_EQ(object.get(), move1.get());
    ASSERT_EQ(nullptr, ptr.get());
    ASSERT_EQ(2, object.use_count());

    Ref<Int> move2 = std::move(move1);
    ASSERT_EQ(object.get(), move2.get());
    ASSERT_EQ(nullptr, move1.get());
    ASSERT_EQ(2, object.use_count());

    move1 = nullptr;
    ASSERT_EQ(2, object.use_count());
    ptr = nullptr;
    ASSERT_EQ(2, object.use_count());
    move2 = nullptr;
    ASSERT_EQ(1, object.use_count());
}

TEST(Ref, canCreateWeakRef) {
    auto object = Valdi::makeShared<Int>(0).toShared();

    Ref<Int> ptr(object.get());

    ASSERT_EQ(2, object.use_count());

    auto weak = weakRef(ptr.get());

    ASSERT_EQ(2, object.use_count());
    ASSERT_FALSE(weak.expired());
    ASSERT_EQ(weak.lock(), object);
}

TEST(SimpleRefCountable, canAllocAndDealloc) {
    auto value = makeShared<Int>(42).toShared();
    auto container = makeShared<Container>();

    ASSERT_EQ(1, value.use_count());
    ASSERT_EQ(1, container.use_count());

    container->value = value;

    ASSERT_EQ(2, value.use_count());
    ASSERT_EQ(1, container.use_count());

    container = nullptr;

    ASSERT_EQ(1, value.use_count());
    ASSERT_EQ(0, container.use_count());
}

} // namespace ValdiTest
