//
//  SkiaBridge.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include <utility>

namespace snap::drawing {

/**
 A BridgedSkValue is a data class which has a memory layout
 100% compatible with a Skia representation.
 Subclasses of BridgedSkValue should expose methods to interact
 with itself without mutating the SkValue. BridgedSkValue
 can be converted into an equivalent Skia value with no
 performance penalty.
 */
template<typename T, typename SkType>
struct BridgedSkValue {
    constexpr SkType& getSkValue() {
        static_assert(sizeof(T) == sizeof(SkType));
        return *reinterpret_cast<SkType*>(this);
    }

    constexpr const SkType& getSkValue() const {
        static_assert(sizeof(T) == sizeof(SkType));
        return *reinterpret_cast<const SkType*>(this);
    }
};

/**
 A WrappedSkValue is an abstraction over a Skia object.
 The Skia object is held internally and can be accessed
 through getSkValue(). Subclasses of WrapedSkValue should
 expose methods to interact with the SKValue, without exposing
 the SkValue itself, thus abstracting it.
 */
template<typename SkType>
class WrappedSkValue {
public:
    template<typename... Args>
    explicit WrappedSkValue(Args&&... args) : _value(std::forward<Args>(args)...) {}

    constexpr SkType& getSkValue() {
        return _value;
    }

    constexpr const SkType& getSkValue() const {
        return _value;
    }

private:
    SkType _value;
};

template<typename T,
         typename SkType,
         typename std::enable_if<std::is_convertible<T*, BridgedSkValue<T, SkType>*>::value, int>::type = 0>
constexpr const T& fromSkValue(const SkType& skValue) {
    return reinterpret_cast<const T&>(skValue);
}

template<typename T,
         typename SkType,
         typename std::enable_if<std::is_convertible<T*, WrappedSkValue<SkType>*>::value, int>::type = 0>
constexpr T fromSkValue(const SkType& skValue) {
    return T(skValue);
}

template<typename T,
         typename SkType,
         typename std::enable_if<std::is_convertible<T*, WrappedSkValue<SkType>*>::value, int>::type = 0>
constexpr T fromSkValue(SkType&& skValue) {
    return T(std::forward<SkType>(skValue));
}

} // namespace snap::drawing
