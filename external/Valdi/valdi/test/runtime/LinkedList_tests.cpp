#include "valdi_core/cpp/Utils/LinkedList.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

class TestNode : public LinkedListNode {};

TEST(LinkedList, canInsertAtBeginning) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    ASSERT_TRUE(list.empty());
    ASSERT_TRUE(list.head() == nullptr);
    ASSERT_TRUE(list.tail() == nullptr);

    list.insert(list.begin(), node1);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node1, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(nullptr, node1->prev());

    list.insert(list.begin(), node2);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node2, list.head());
    ASSERT_EQ(node1, list.tail());
    ASSERT_EQ(nullptr, node2->prev());
    ASSERT_EQ(node1.get(), node2->next());
    ASSERT_EQ(node2.get(), node1->prev());
    ASSERT_EQ(nullptr, node1->next());

    list.insert(list.begin(), node3);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node3, list.head());
    ASSERT_EQ(node1, list.tail());
    ASSERT_EQ(nullptr, node3->prev());
    ASSERT_EQ(node2.get(), node3->next());
    ASSERT_EQ(node3.get(), node2->prev());
    ASSERT_EQ(node1.get(), node2->next());
    ASSERT_EQ(node2.get(), node1->prev());
    ASSERT_EQ(nullptr, node1->next());
}

TEST(LinkedList, canInsertAtEnd) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    ASSERT_TRUE(list.empty());
    ASSERT_TRUE(list.head() == nullptr);
    ASSERT_TRUE(list.tail() == nullptr);

    list.insert(list.end(), node1);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node1, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(nullptr, node1->prev());

    list.insert(list.end(), node2);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node2, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(node2.get(), node1->next());
    ASSERT_EQ(node1.get(), node2->prev());
    ASSERT_EQ(nullptr, node2->next());

    list.insert(list.end(), node3);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node3, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(node2.get(), node1->next());
    ASSERT_EQ(node1.get(), node2->prev());
    ASSERT_EQ(node3.get(), node2->next());
    ASSERT_EQ(node2.get(), node3->prev());
    ASSERT_EQ(nullptr, node3->next());
}

TEST(LinkedList, canInsertAtMiddle) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.begin(), node1);
    list.insert(list.end(), node2);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node2, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(node2.get(), node1->next());
    ASSERT_EQ(node1.get(), node2->prev());
    ASSERT_EQ(nullptr, node2->next());

    list.insertAfter(node1, node3);

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node2, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(node3.get(), node1->next());
    ASSERT_EQ(node1.get(), node3->prev());
    ASSERT_EQ(node2.get(), node3->next());
    ASSERT_EQ(node3.get(), node2->prev());
    ASSERT_EQ(nullptr, node2->next());
}

TEST(LinkedList, canRemoveAtBeginning) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.end(), node1);
    list.insert(list.end(), node2);
    list.insert(list.end(), node3);

    list.remove(list.begin());

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node2, list.head());
    ASSERT_EQ(node3, list.tail());
    ASSERT_EQ(nullptr, node2->prev());
    ASSERT_EQ(node3.get(), node2->next());
    ASSERT_EQ(node2.get(), node3->prev());
    ASSERT_EQ(nullptr, node3->next());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(nullptr, node1->next());

    list.remove(list.begin());

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node3, list.head());
    ASSERT_EQ(node3, list.tail());
    ASSERT_EQ(nullptr, node3->prev());
    ASSERT_EQ(nullptr, node3->next());
    ASSERT_EQ(nullptr, node2->prev());
    ASSERT_EQ(nullptr, node2->next());

    list.remove(list.begin());

    ASSERT_TRUE(list.empty());
    ASSERT_EQ(nullptr, list.head());
    ASSERT_EQ(nullptr, list.tail());
    ASSERT_EQ(nullptr, node3->prev());
    ASSERT_EQ(nullptr, node3->next());
}

TEST(LinkedList, canRemoveAtEnd) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.end(), node1);
    list.insert(list.end(), node2);
    list.insert(list.end(), node3);

    list.remove(list.last());

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node2, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(node2.get(), node1->next());
    ASSERT_EQ(node1.get(), node2->prev());
    ASSERT_EQ(nullptr, node2->next());
    ASSERT_EQ(nullptr, node3->prev());
    ASSERT_EQ(nullptr, node3->next());

    list.remove(list.last());

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node1, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(nullptr, node1->next());
    ASSERT_EQ(nullptr, node2->prev());
    ASSERT_EQ(nullptr, node2->next());

    list.remove(list.begin());

    ASSERT_TRUE(list.empty());
    ASSERT_EQ(nullptr, list.head());
    ASSERT_EQ(nullptr, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(nullptr, node1->next());
}

TEST(LinkedList, canRemoveAtMiddle) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.end(), node1);
    list.insert(list.end(), node2);
    list.insert(list.end(), node3);

    list.remove(list.iterator(node2));

    ASSERT_FALSE(list.empty());
    ASSERT_EQ(node1, list.head());
    ASSERT_EQ(node3, list.tail());
    ASSERT_EQ(nullptr, node1->prev());
    ASSERT_EQ(node3.get(), node1->next());
    ASSERT_EQ(node1.get(), node3->prev());
    ASSERT_EQ(nullptr, node3->next());
    ASSERT_EQ(nullptr, node2->prev());
    ASSERT_EQ(nullptr, node2->next());
}

TEST(LinkedList, canIterate) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.end(), node1);
    list.insert(list.end(), node2);
    list.insert(list.end(), node3);

    std::vector<Ref<TestNode>> nodes;
    for (const auto& node : list) {
        nodes.emplace_back(node);
    }

    ASSERT_EQ(static_cast<size_t>(3), nodes.size());
    ASSERT_EQ(node1, nodes[0]);
    ASSERT_EQ(node2, nodes[1]);
    ASSERT_EQ(node3, nodes[2]);
}

TEST(LinkedList, canIterateBackward) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.end(), node1);
    list.insert(list.end(), node2);
    list.insert(list.end(), node3);

    std::vector<Ref<TestNode>> nodes;
    auto it = list.rbegin();
    while (it != list.rend()) {
        nodes.emplace_back(*it);
        it++;
    }

    ASSERT_EQ(static_cast<size_t>(3), nodes.size());
    ASSERT_EQ(node3, nodes[0]);
    ASSERT_EQ(node2, nodes[1]);
    ASSERT_EQ(node1, nodes[2]);
}

TEST(LinkedList, releaseAndRetainNodes) {
    LinkedList<TestNode> list;

    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    list.insert(list.end(), node1);

    // head, tail, and in the test
    ASSERT_EQ(3, node1.use_count());

    list.insert(list.end(), node2);

    // Head
    ASSERT_EQ(2, node1.use_count());
    // Tail, next of node1
    ASSERT_EQ(3, node2.use_count());

    list.insert(list.end(), node3);

    // Head
    ASSERT_EQ(2, node1.use_count());
    // Next of node1
    ASSERT_EQ(2, node2.use_count());
    // Tail, Next of node2
    ASSERT_EQ(3, node3.use_count());

    list.remove(list.last());

    // Head
    ASSERT_EQ(2, node1.use_count());
    // Tail, next of node1
    ASSERT_EQ(3, node2.use_count());

    ASSERT_EQ(1, node3.use_count());

    list.remove(list.begin());

    ASSERT_EQ(1, node1.use_count());
    // Head, Tail
    ASSERT_EQ(3, node2.use_count());

    ASSERT_EQ(1, node3.use_count());

    list.remove(list.iterator(node2));

    ASSERT_EQ(1, node1.use_count());
    // Head, Tail
    ASSERT_EQ(1, node2.use_count());

    ASSERT_EQ(1, node3.use_count());
}

TEST(LinkedList, releaseNodeOnDestruct) {
    auto node1 = makeShared<TestNode>();
    auto node2 = makeShared<TestNode>();
    auto node3 = makeShared<TestNode>();

    {
        LinkedList<TestNode> list;

        list.insert(list.end(), node1);
        list.insert(list.end(), node2);
        list.insert(list.end(), node3);

        // Head
        ASSERT_EQ(2, node1.use_count());
        // Next of node1
        ASSERT_EQ(2, node2.use_count());
        // Tail, Next of node2
        ASSERT_EQ(3, node3.use_count());
    }

    ASSERT_EQ(1, node1.use_count());
    ASSERT_EQ(1, node2.use_count());
    ASSERT_EQ(1, node3.use_count());
}

} // namespace ValdiTest
