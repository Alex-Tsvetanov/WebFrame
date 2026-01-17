//
//  LinkedList.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/15/22.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

template<typename NodeType>
class LinkedList;

/**
 Base class for a node within a LinkedList.
 Consumers of the LinkedList class should inherit this node
 to make it insertable within a LinkedList.
 */
class LinkedListNode : public SimpleRefCountable {
public:
    LinkedListNode();
    ~LinkedListNode() override;

    LinkedListNode* next() const;
    LinkedListNode* prev() const;

private:
    template<typename NodeType>
    friend class LinkedList;

    LinkedListNode* _prev = nullptr;
    Ref<LinkedListNode> _next;

    void remove();

    void insert(LinkedListNode* prev, LinkedListNode* next);
};

template<typename NodeType>
class LinkedListIteratorBase {
public:
    explicit LinkedListIteratorBase(const Ref<NodeType>& node) : _node(node) {}
    LinkedListIteratorBase() = default;

    ~LinkedListIteratorBase() = default;

    constexpr bool operator==(const LinkedListIteratorBase<NodeType>& iterator) const {
        return _node == iterator._node;
    }

    constexpr bool operator!=(const LinkedListIteratorBase<NodeType>& iterator) const {
        return _node != iterator._node;
    }

    constexpr const Ref<NodeType>& operator*() const {
        return _node;
    }

    constexpr const Ref<NodeType>& operator->() const {
        return _node;
    }

protected:
    void next() {
        _node = static_cast<NodeType*>(_node->next());
    }

    void back() {
        _node = static_cast<NodeType*>(_node->prev());
    }

private:
    Ref<NodeType> _node;
};

/**
 Reverse Iterator for the LinkedList class
 */
template<typename NodeType>
class LinkedListReverseIterator : public LinkedListIteratorBase<NodeType> {
public:
    explicit LinkedListReverseIterator(const Ref<NodeType>& node) : LinkedListIteratorBase<NodeType>(node) {}
    LinkedListReverseIterator() = default;

    LinkedListReverseIterator<NodeType>& operator++() {
        this->back();
        return *this;
    }

    LinkedListReverseIterator<NodeType>& operator++(int /*unused*/) {
        this->back();
        return *this;
    }

    LinkedListReverseIterator<NodeType>& operator--() {
        this->next();
        return *this;
    }

    LinkedListReverseIterator<NodeType>& operator--(int /*unused*/) {
        this->next();
        return *this;
    }
};

/**
 Iterator for the LinkedList class
 */
template<typename NodeType>
class LinkedListIterator : public LinkedListIteratorBase<NodeType> {
public:
    explicit LinkedListIterator(const Ref<NodeType>& node) : LinkedListIteratorBase<NodeType>(node) {}
    LinkedListIterator() = default;

    LinkedListIterator<NodeType>& operator++() {
        this->next();
        return *this;
    }

    LinkedListIterator<NodeType>& operator++(int /*unused*/) {
        this->next();
        return *this;
    }

    LinkedListIterator<NodeType>& operator--() {
        this->back();
        return *this;
    }

    LinkedListIterator<NodeType>& operator--(int /*unused*/) {
        this->back();
        return *this;
    }
};

/**
 LinkedList is an efficient inline LinkedList implementation, allowing
 objects to be inserted in a list with no additional heap allocations
 unlike std::list . The node within the LinkedList is allocated by the
 consumer of this class, and the node class needs to inherit LinkedListNode.
 */
template<typename NodeType>
class LinkedList : public snap::NonCopyable {
public:
    using Iterator = LinkedListIterator<NodeType>;
    using ReverseIterator = LinkedListReverseIterator<NodeType>;

    ~LinkedList() {
        clear();
    }

    constexpr const Ref<NodeType>& head() const {
        return _head;
    }

    constexpr const Ref<NodeType>& tail() const {
        return _tail;
    }

    Iterator begin() const {
        return Iterator(_head);
    }

    Iterator end() const {
        return Iterator();
    }

    ReverseIterator rbegin() const {
        return ReverseIterator(_tail);
    }

    ReverseIterator rend() const {
        return ReverseIterator();
    }

    Iterator last() const {
        return Iterator(_tail);
    }

    Iterator iterator(const Ref<NodeType>& node) {
        return Iterator(node);
    }

    ReverseIterator reverseIterator(const Ref<NodeType>& node) {
        return ReverseIterator(node);
    }

    Iterator insertBefore(const Ref<NodeType>& beforeNode, const Ref<NodeType>& node) {
        return insert(iterator(beforeNode), node);
    }

    Iterator insertAfter(const Ref<NodeType>& afterNode, const Ref<NodeType>& node) {
        auto it = iterator(afterNode);
        it++;
        return insert(it, node);
    }

    void pushFront(Ref<NodeType> node) {
        if (node == _head) {
            return;
        }

        removeNode(node);
        node->insert(nullptr, _head.get());

        _head = std::move(node);

        if (_head->next() == nullptr) {
            _tail = _head;
        }
    }

    Iterator insert(const LinkedListIteratorBase<NodeType>& iterator, const Ref<NodeType>& node) {
        removeNode(node);

        if (iterator == end()) {
            // Insert at end
            node->insert(_tail.get(), nullptr);
        } else {
            node->insert(iterator->prev(), (*iterator).get());
        }

        if (node->prev() == nullptr) {
            _head = node;
        }

        if (node->next() == nullptr) {
            _tail = node;
        }

        return Iterator(node);
    }

    void remove(const LinkedListIteratorBase<NodeType>& iterator) {
        removeNode(*iterator);
    }

    Iterator erase(const Iterator& iterator) {
        auto next = iterator;
        next++;
        removeNode(*iterator);
        return next;
    }

    ReverseIterator erase(const ReverseIterator& iterator) {
        auto next = iterator;
        next++;
        removeNode(*iterator);
        return next;
    }

    bool empty() const {
        return _head == nullptr;
    }

    void clear() {
        while (_tail != nullptr) {
            auto node = _tail;
            removeNode(node);
        }
    }

private:
    Ref<NodeType> _head;
    Ref<NodeType> _tail;

    void removeNode(const Ref<NodeType>& node) {
        if (_head == node) {
            _head = static_cast<NodeType*>(node->next());
        }

        if (_tail == node) {
            _tail = static_cast<NodeType*>(node->prev());
        }

        node->remove();
    }
};

} // namespace Valdi
