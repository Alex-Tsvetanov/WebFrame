#pragma once

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#include <type_traits>
#include <utility>

#ifdef __cpp_lib_expected

#include <expected>

namespace djinni {

using ::std::expected;
using ::std::unexpected;
inline constexpr ::std::unexpect_t unexpect{};

} // namespace djinni

#else

#include "tl_expected.hpp"

namespace djinni {

using ::tl::expected;
using ::tl::unexpected;
inline constexpr ::tl::unexpect_t unexpect{};

} // namespace djinni

#endif

namespace djinni {

template<class E>
djinni::unexpected<typename std::decay<E>::type> make_unexpected(E&& e) {
    return djinni::unexpected<typename std::decay<E>::type>(std::forward<E>(e));
}

} // namespace djinni
