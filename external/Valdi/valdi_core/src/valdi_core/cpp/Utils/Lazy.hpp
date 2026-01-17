//
//  Lazy.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 5/3/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include <optional>

namespace Valdi {

template<typename T>
class Lazy {
public:
    using Factory = Function<T()>;

    explicit Lazy() = default;
    explicit Lazy(Factory factory) : _factory(std::move(factory)) {}
    ~Lazy() = default;

    /**
     Return the value held by the Lazy.
     Create the value through the factory if the value was never created.
     */
    const T& getOrCreate() {
        std::lock_guard<Mutex> guard(_mutex);
        if (!_value) {
            _value = {_factory()};
            _factory = Factory();
        }

        return _value.value();
    }

    /**
     Return the value as an optional, which will be nullopt
     if the value was not created
     */
    std::optional<T> getIfCreated() const {
        std::lock_guard<Mutex> guard(_mutex);
        return _value;
    }

    bool hasValue() const {
        std::lock_guard<Mutex> guard(_mutex);
        return _value.has_value();
    }

    void setFactory(Factory&& factory) {
        std::lock_guard<Mutex> guard(_mutex);
        _factory = std::move(factory);
    }

private:
    mutable Mutex _mutex;
    Factory _factory;
    std::optional<T> _value;
};

} // namespace Valdi
