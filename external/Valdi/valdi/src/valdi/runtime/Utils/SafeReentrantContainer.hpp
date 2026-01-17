//
//  SafeReentrantContainer.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/16/19.
//

#pragma once

#include "utils/debugging/Assert.hpp"
#include <utility>

namespace Valdi {

template<typename T, typename Iterator>
class SafeReentrantContainerIterator {
public:
    SafeReentrantContainerIterator(T* container, int mutationId, Iterator iterator)
        : _container(container), _mutationId(mutationId), _iterator(std::move(iterator)) {}

    SafeReentrantContainerIterator<T, Iterator>& operator=(const SafeReentrantContainerIterator<T, Iterator>& other) {
        _container = other._container;
        _mutationId = other._mutationId;
        _iterator = other._iterator;
        return *this;
    }

    SafeReentrantContainerIterator<T, Iterator>& operator++() {
        if (_mutationId == _container->getMutationId()) {
            _iterator++;
        } else {
            *this = _container->begin();
        }
        return *this;
    }

    SafeReentrantContainerIterator<T, Iterator> operator++(int /* unused */) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const SafeReentrantContainerIterator<T, Iterator>& other) const {
        return _iterator == other._iterator;
    }

    bool operator!=(const SafeReentrantContainerIterator<T, Iterator>& other) const {
        return _iterator != other._iterator;
    }

    auto operator*() const {
        return *_iterator;
    }

    auto operator->() const {
        return _iterator;
    }

    int getMutationId() const {
        return _mutationId;
    }

    Iterator& getInnerIterator() {
        return _iterator;
    }

    const Iterator& getInnerIterator() const {
        return _iterator;
    }

private:
    T* _container;
    int _mutationId;
    Iterator _iterator;
};

/**
 An abstraction over a container allowing mutations during iteration.
 If a mutation is found during iteration, iteration will start from the beginning.
 There are therefore no guarantee that each item will be only processed once
 during iteration.
 */
template<typename T>
class SafeReentrantContainer {
public:
    template<typename F>
    void mutate(F&& mutateBlock) {
        _mutationId++;
        mutateBlock(_container);
    }

    void clear() {
        _mutationId++;
        _container.clear();
    }

    template<typename Iterator>
    auto erase(Iterator& iterator) {
        SC_ASSERT(iterator.getMutationId() == _mutationId, "Iterator was already invalidated");
        _mutationId++;

        return SafeReentrantContainerIterator(this, _mutationId, _container.erase(iterator.getInnerIterator()));
    }

    template<typename Key>
    auto find(const Key& key) {
        return SafeReentrantContainerIterator(this, _mutationId, _container.find(key));
    }

    template<typename Key>
    auto find(const Key& key) const {
        return SafeReentrantContainerIterator(this, _mutationId, _container.find(key));
    }

    auto begin() {
        return SafeReentrantContainerIterator(this, _mutationId, _container.begin());
    }

    auto end() {
        return SafeReentrantContainerIterator(this, _mutationId, _container.end());
    }

    auto begin() const {
        return SafeReentrantContainerIterator(this, _mutationId, _container.begin());
    }

    auto end() const {
        return SafeReentrantContainerIterator(this, _mutationId, _container.end());
    }

    int getMutationId() const {
        return _mutationId;
    }

private:
    T _container;
    int _mutationId = 0;
};

} // namespace Valdi
