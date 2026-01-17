//
//  LRUCache.hpp
//  valdi_core
//
//  Created by Simon Corsin on 11/15/22.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/LinkedList.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include <utility>

namespace Valdi {

template<typename Key, typename Value>
class LRUCacheNode : public LinkedListNode {
public:
    explicit LRUCacheNode(std::pair<Key, Value>&& keyValue) : _keyValue(std::move(keyValue)) {}

    ~LRUCacheNode() override = default;

    constexpr const Key& key() const {
        return _keyValue.first;
    }

    constexpr Key& key() {
        return _keyValue.first;
    }

    constexpr const Value& value() const {
        return _keyValue.second;
    }

    constexpr Value& value() {
        return _keyValue.second;
    }

    constexpr const std::pair<Key, Value>& keyValue() const {
        return _keyValue;
    }

private:
    std::pair<Key, Value> _keyValue;
};

/**
 An efficient LRUCache implementation that uses an inline linked list
 and can insert preallocated nodes.
 */
template<typename Key, typename Value>
class LRUCache {
public:
    using Node = LRUCacheNode<Key, Value>;
    using Iterator = typename LinkedList<Node>::Iterator;

    explicit LRUCache(size_t capacity) : _capacity(capacity) {}

    ~LRUCache() = default;

    size_t size() const {
        return _nodeByKey.size();
    }

    bool empty() const {
        return _nodeByKey.empty();
    }

    void clear() {
        _nodeByKey.clear();
        _list.clear();
    }

    void setCapacity(size_t capacity) {
        _capacity = capacity;
        trimToCapacity();
    }

    bool remove(const Key& key) {
        auto it = _nodeByKey.find(key);
        if (it == _nodeByKey.end()) {
            return false;
        }

        auto node = std::move(it->second);
        _nodeByKey.erase(it);

        _list.remove(_list.iterator(node));

        return true;
    }

    bool contains(const Key& key) const {
        return _nodeByKey.find(key) != _nodeByKey.end();
    }

    Iterator find(const Key& key) {
        const auto& it = _nodeByKey.find(key);
        if (it == _nodeByKey.end()) {
            return end();
        }

        return insertNodeInList(it->second);
    }

    Iterator insert(Key&& key, Value&& value) {
        return insert(std::make_pair(std::move(key), std::move(value)));
    }

    Iterator insert(const Key& key, const Value& value) {
        return insert(std::make_pair(key, value));
    }

    Iterator insert(std::pair<Key, Value>&& keyValue) {
        auto it = _nodeByKey.find(keyValue.first);
        if (it != _nodeByKey.end()) {
            // Node exist in list, we just update its value
            it->second->value() = std::move(keyValue.second);
            return insertNodeInList(it->second);
        }

        // Node doesn't exist
        auto node = makeShared<Node>(std::move(keyValue));
        return insert(node);
    }

    Iterator insert(const Ref<Node>& node) {
        _nodeByKey[node->key()] = node;

        auto iterator = insertNodeInList(node);

        trimToCapacity();

        return iterator;
    }

    Iterator begin() const {
        return _list.begin();
    }

    Iterator end() const {
        return _list.end();
    }

private:
    FlatMap<Key, Ref<Node>> _nodeByKey;
    LinkedList<Node> _list;
    size_t _capacity;

    void trimToCapacity() {
        while (size() > _capacity) {
            remove(_list.tail()->key());
        }
    }

    Iterator insertNodeInList(Ref<Node> node) {
        _list.pushFront(std::move(node));
        return _list.begin();
    }
};

} // namespace Valdi
