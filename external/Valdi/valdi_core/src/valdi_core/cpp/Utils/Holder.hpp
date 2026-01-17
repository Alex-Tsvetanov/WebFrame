#pragma once

#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <optional>

namespace Valdi {

template<typename T>
struct ContainerHolder : public SimpleRefCountable {
    Mutex mutex;
    T value;
};

/**
 An object which can store and fetch a value under a mutex.
 It provides an indirection to enable multiple consumers to
 read a value from the same memory location safely.
 The Holder object can be copied and shared across threads.
 */
template<typename T>
class Holder {
public:
    Holder() : _container(makeShared<ContainerHolder<T>>()) {}
    ~Holder() = default;

    void set(const T& value) const {
        std::lock_guard<Mutex> guard(_container->mutex);
        _container->value = value;
    }

    void set(T&& value) const {
        std::lock_guard<Mutex> guard(_container->mutex);
        _container->value = std::move(value);
    }

    T get() const {
        std::lock_guard<Mutex> guard(_container->mutex);
        return _container->value;
    }

    template<typename Fn>
    void access(Fn&& fn) const {
        std::lock_guard<Mutex> guard(_container->mutex);
        fn(_container->value);
    }

private:
    Ref<ContainerHolder<T>> _container;
};

} // namespace Valdi