//
//  ObjectPool.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/4/19.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace Valdi {

template<typename T>
class ObjectPoolInner;

template<typename T>
class ObjectPool {
public:
    static ObjectPoolInner<T>& get() {
        static auto innerPool = new ObjectPoolInner<T>();
        return *innerPool;
    }
};

template<typename T, typename Cleanup>
struct ObjectPoolEntry {
    ObjectPoolEntry(T value, ObjectPoolInner<T>* pool, Cleanup cleanUpFunc)
        : _value(std::move(value)), _pool(pool), _cleanUpFunc(std::move(cleanUpFunc)) {}

    ObjectPoolEntry(ObjectPoolEntry<T, Cleanup>&& other) noexcept
        : _value(std::move(other._value)), _pool(other._pool), _cleanUpFunc(std::move(other._cleanUpFunc)) {
        other._pool = nullptr;
    }

    ObjectPoolEntry(const ObjectPoolEntry<T, Cleanup>& other) = delete;

    ~ObjectPoolEntry() {
        release();
    }

    ObjectPoolEntry<T, Cleanup>& operator=(ObjectPoolEntry<T, Cleanup>&& other) noexcept {
        release();

        _value = std::move(other._value);
        _cleanUpFunc = std::move(other._cleanUpFunc);
        _pool = other._pool;
        other._pool = nullptr;

        return *this;
    }

    ObjectPoolEntry<T, Cleanup>& operator=(const ObjectPoolEntry<T, Cleanup>& other) = delete;

    T* operator->() {
        return &_value;
    }

    const T* operator->() const {
        return &_value;
    }

    T& operator*() {
        return _value;
    }

    const T& operator*() const {
        return _value;
    }

    T* get() {
        return &_value;
    }

    const T* get() const {
        return &_value;
    }

private:
    T _value;
    ObjectPoolInner<T>* _pool;
    Cleanup _cleanUpFunc;

    void release() {
        if (_pool != nullptr) {
            _cleanUpFunc(_value);
            _pool->add(std::move(_value));
            _pool = nullptr;
        }
    }
};

template<typename T>
class ObjectPoolInner {
public:
    template<typename Cleanup>
    ObjectPoolEntry<T, Cleanup> getOrCreate(Cleanup&& cleanup) {
        return getOrCreate([]() { return T(); }, std::forward<Cleanup>(cleanup));
    }

    template<typename Factory, typename Cleanup>
    ObjectPoolEntry<T, Cleanup> getOrCreate(Factory&& factory, Cleanup&& cleanup) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_objects.empty()) {
            return ObjectPoolEntry<T, Cleanup>(factory(), this, cleanup);
        }

        T obj = std::move(_objects.front());
        _objects.pop_front();

        return ObjectPoolEntry<T, Cleanup>(std::move(obj), this, cleanup);
    }

    void add(T object) {
        std::lock_guard<std::mutex> lock(_mutex);

        _objects.emplace_back(std::move(object));
    }

private:
    std::deque<T> _objects;
    std::mutex _mutex;
};

template<typename T>
void cleanUpArray(std::vector<T>& array) {
    array.clear();
}

template<typename T>
using ReusableArray = ObjectPoolEntry<std::vector<T>, void (*)(std::vector<T>&)>;

template<typename T>
ReusableArray<T> makeReusableArray() {
    return ObjectPool<std::vector<T>>::get().getOrCreate(&cleanUpArray<T>);
}

} // namespace Valdi
