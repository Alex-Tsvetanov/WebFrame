/**
 *  @file   lambda2function.hpp
 *  @brief  Util for saving lambda functions into variables
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#include <functional>
#include <type_traits>

namespace detail {
template <typename T> struct deduce_type;

template <typename RETURN_TYPE, typename CLASS_TYPE, typename... ARGS>
struct deduce_type<RETURN_TYPE (CLASS_TYPE::*)(ARGS...) const> {
  using type = std::function<RETURN_TYPE(ARGS...)>;
};
} // namespace detail

template <typename CLOSURE> auto wrap(const CLOSURE &fn) {
  return typename detail::deduce_type<decltype(&CLOSURE::operator())>::type(fn);
}