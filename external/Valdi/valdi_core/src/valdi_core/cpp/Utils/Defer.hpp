//
//  Defer.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/30/19.
//

#pragma once

#include <utility>

namespace Valdi {

template<typename T>
class Defer {
public:
    explicit Defer(T&& deferFunc) : _deferFunc(std::move(deferFunc)) {}

    ~Defer() {
        _deferFunc();
    }

private:
    T _deferFunc;
};

} // namespace Valdi
