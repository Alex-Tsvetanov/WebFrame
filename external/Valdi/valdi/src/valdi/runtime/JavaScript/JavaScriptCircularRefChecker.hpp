//
//  JavaScriptCircularRefChecker.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 12/5/18.
//

#pragma once

#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace Valdi {

template<typename T>
class JavaScriptCircularRefChecker;

template<typename T>
class JavaScriptRef {
public:
    JavaScriptRef() : _refChecker(nullptr) {}
    explicit JavaScriptRef(JavaScriptCircularRefChecker<T>* refChecker) : _refChecker(refChecker) {}

    JavaScriptRef(const JavaScriptRef<T>&) = delete;
    JavaScriptRef<T>& operator=(const JavaScriptRef<T>& other) = delete;

    JavaScriptRef<T>& operator=(JavaScriptRef<T>&& other) noexcept {
        _refChecker = other._refChecker;
        other._refChecker = nullptr;

        return *this;
    }

    ~JavaScriptRef() {
        if (_refChecker != nullptr) {
            _refChecker->_values.pop_back();
        }
    }

private:
    JavaScriptCircularRefChecker<T>* _refChecker;
};

template<typename T>
class JavaScriptCircularRefChecker {
public:
    friend JavaScriptRef<T>;

    bool isRefAlreadyPresent(const T& value) const {
        for (const auto& existingValue : _values) {
            if (existingValue == value) {
                return true;
            }
        }

        return false;
    }

    JavaScriptRef<T> push(const T& value) {
        _values.emplace_back(value);
        return JavaScriptRef<T>(this);
    }

private:
    SmallVector<T, 8> _values;
};

} // namespace Valdi
