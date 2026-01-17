#pragma once

// Use std::expected if available (C++23), otherwise our implementation

#if defined(COROUTE_HAS_STD_EXPECTED) || __cpp_lib_expected >= 202202L

#include <expected>

namespace coroute {
    using std::expected;
    using std::unexpected;
    using std::unexpect;
    using std::unexpect_t;
}

#else

#include <variant>
#include <utility>
#include <type_traits>
#include <exception>
#include <functional>

namespace coroute {

// Tag type for in-place unexpected construction
struct unexpect_t {
    explicit unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

// Wrapper for unexpected value
template<typename E>
class unexpected {
    E error_;

public:
    constexpr unexpected(const unexpected&) = default;
    constexpr unexpected(unexpected&&) = default;
    
    template<typename Err = E>
        requires std::is_constructible_v<E, Err>
    constexpr explicit unexpected(Err&& e)
        : error_(std::forward<Err>(e)) {}
    
    constexpr const E& error() const& noexcept { return error_; }
    constexpr E& error() & noexcept { return error_; }
    constexpr const E&& error() const&& noexcept { return std::move(error_); }
    constexpr E&& error() && noexcept { return std::move(error_); }
};

template<typename E>
unexpected(E) -> unexpected<E>;

// Exception thrown when accessing value of error expected
template<typename E>
class bad_expected_access;

template<>
class bad_expected_access<void> : public std::exception {
public:
    const char* what() const noexcept override {
        return "bad expected access";
    }
};

template<typename E>
class bad_expected_access : public bad_expected_access<void> {
    E error_;
public:
    explicit bad_expected_access(E e) : error_(std::move(e)) {}
    const E& error() const& noexcept { return error_; }
    E& error() & noexcept { return error_; }
};

// Main expected class
template<typename T, typename E>
class expected {
    static_assert(!std::is_same_v<T, std::remove_cv_t<unexpected<E>>>,
                  "T cannot be unexpected<E>");
    static_assert(!std::is_same_v<T, unexpect_t>,
                  "T cannot be unexpect_t");

    std::variant<T, unexpected<E>> data_;

    bool has_val() const noexcept {
        return data_.index() == 0;
    }

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    // Constructors
    constexpr expected() 
        requires std::is_default_constructible_v<T>
        : data_(std::in_place_index<0>) {}

    constexpr expected(const expected&) = default;
    constexpr expected(expected&&) = default;

    template<typename U = T>
        requires std::is_constructible_v<T, U> &&
                 (!std::is_same_v<std::remove_cvref_t<U>, expected>) &&
                 (!std::is_same_v<std::remove_cvref_t<U>, unexpected<E>>)
    constexpr expected(U&& v)
        : data_(std::in_place_index<0>, std::forward<U>(v)) {}

    template<typename G>
        requires std::is_constructible_v<E, const G&>
    constexpr expected(const unexpected<G>& e)
        : data_(std::in_place_index<1>, unexpected<E>(e.error())) {}

    template<typename G>
        requires std::is_constructible_v<E, G>
    constexpr expected(unexpected<G>&& e)
        : data_(std::in_place_index<1>, unexpected<E>(std::move(e.error()))) {}

    template<typename... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit expected(std::in_place_t, Args&&... args)
        : data_(std::in_place_index<0>, std::forward<Args>(args)...) {}

    template<typename... Args>
        requires std::is_constructible_v<E, Args...>
    constexpr explicit expected(unexpect_t, Args&&... args)
        : data_(std::in_place_index<1>, unexpected<E>(std::forward<Args>(args)...)) {}

    // Assignment
    expected& operator=(const expected&) = default;
    expected& operator=(expected&&) = default;

    template<typename U = T>
        requires (!std::is_same_v<expected, std::remove_cvref_t<U>>) &&
                 std::is_constructible_v<T, U> &&
                 std::is_assignable_v<T&, U>
    expected& operator=(U&& v) {
        data_.template emplace<0>(std::forward<U>(v));
        return *this;
    }

    template<typename G>
    expected& operator=(const unexpected<G>& e) {
        data_.template emplace<1>(unexpected<E>(e.error()));
        return *this;
    }

    template<typename G>
    expected& operator=(unexpected<G>&& e) {
        data_.template emplace<1>(unexpected<E>(std::move(e.error())));
        return *this;
    }

    // Observers
    constexpr bool has_value() const noexcept { return has_val(); }
    constexpr explicit operator bool() const noexcept { return has_val(); }

    constexpr const T* operator->() const noexcept {
        return std::addressof(std::get<0>(data_));
    }
    constexpr T* operator->() noexcept {
        return std::addressof(std::get<0>(data_));
    }

    constexpr const T& operator*() const& noexcept {
        return std::get<0>(data_);
    }
    constexpr T& operator*() & noexcept {
        return std::get<0>(data_);
    }
    constexpr const T&& operator*() const&& noexcept {
        return std::move(std::get<0>(data_));
    }
    constexpr T&& operator*() && noexcept {
        return std::move(std::get<0>(data_));
    }

    constexpr const T& value() const& {
        if (!has_val()) throw bad_expected_access(error());
        return std::get<0>(data_);
    }
    constexpr T& value() & {
        if (!has_val()) throw bad_expected_access(error());
        return std::get<0>(data_);
    }
    constexpr const T&& value() const&& {
        if (!has_val()) throw bad_expected_access(std::move(error()));
        return std::move(std::get<0>(data_));
    }
    constexpr T&& value() && {
        if (!has_val()) throw bad_expected_access(std::move(error()));
        return std::move(std::get<0>(data_));
    }

    constexpr const E& error() const& noexcept {
        return std::get<1>(data_).error();
    }
    constexpr E& error() & noexcept {
        return std::get<1>(data_).error();
    }
    constexpr const E&& error() const&& noexcept {
        return std::move(std::get<1>(data_).error());
    }
    constexpr E&& error() && noexcept {
        return std::move(std::get<1>(data_).error());
    }

    template<typename U>
    constexpr T value_or(U&& default_value) const& {
        return has_val() ? std::get<0>(data_) 
                         : static_cast<T>(std::forward<U>(default_value));
    }
    template<typename U>
    constexpr T value_or(U&& default_value) && {
        return has_val() ? std::move(std::get<0>(data_))
                         : static_cast<T>(std::forward<U>(default_value));
    }

    // Monadic operations
    template<typename F>
    constexpr auto and_then(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if (has_val()) return std::invoke(std::forward<F>(f), **this);
        return U(unexpect, error());
    }

    template<typename F>
    constexpr auto and_then(F&& f) const& {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_val()) return std::invoke(std::forward<F>(f), **this);
        return U(unexpect, error());
    }

    template<typename F>
    constexpr auto and_then(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_val()) return std::invoke(std::forward<F>(f), std::move(**this));
        return U(unexpect, std::move(error()));
    }

    template<typename F>
    constexpr auto transform(F&& f) & {
        using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
        if (has_val()) return expected<U, E>(std::invoke(std::forward<F>(f), **this));
        return expected<U, E>(unexpect, error());
    }

    template<typename F>
    constexpr auto transform(F&& f) const& {
        using U = std::remove_cv_t<std::invoke_result_t<F, const T&>>;
        if (has_val()) return expected<U, E>(std::invoke(std::forward<F>(f), **this));
        return expected<U, E>(unexpect, error());
    }

    template<typename F>
    constexpr auto transform(F&& f) && {
        using U = std::remove_cv_t<std::invoke_result_t<F, T&&>>;
        if (has_val()) return expected<U, E>(std::invoke(std::forward<F>(f), std::move(**this)));
        return expected<U, E>(unexpect, std::move(error()));
    }

    template<typename F>
    constexpr auto or_else(F&& f) & {
        using G = std::remove_cvref_t<std::invoke_result_t<F, E&>>;
        if (has_val()) return G(**this);
        return std::invoke(std::forward<F>(f), error());
    }

    template<typename F>
    constexpr auto or_else(F&& f) const& {
        using G = std::remove_cvref_t<std::invoke_result_t<F, const E&>>;
        if (has_val()) return G(**this);
        return std::invoke(std::forward<F>(f), error());
    }

    template<typename F>
    constexpr auto or_else(F&& f) && {
        using G = std::remove_cvref_t<std::invoke_result_t<F, E&&>>;
        if (has_val()) return G(std::move(**this));
        return std::invoke(std::forward<F>(f), std::move(error()));
    }
};

// Specialization for void
template<typename E>
class expected<void, E> {
    std::variant<std::monostate, unexpected<E>> data_;

    bool has_val() const noexcept {
        return data_.index() == 0;
    }

public:
    using value_type = void;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    constexpr expected() noexcept : data_(std::monostate{}) {}
    constexpr expected(const expected&) = default;
    constexpr expected(expected&&) = default;

    template<typename G>
        requires std::is_constructible_v<E, const G&>
    constexpr expected(const unexpected<G>& e)
        : data_(std::in_place_index<1>, unexpected<E>(e.error())) {}

    template<typename G>
        requires std::is_constructible_v<E, G>
    constexpr expected(unexpected<G>&& e)
        : data_(std::in_place_index<1>, unexpected<E>(std::move(e.error()))) {}

    template<typename... Args>
        requires std::is_constructible_v<E, Args...>
    constexpr explicit expected(unexpect_t, Args&&... args)
        : data_(std::in_place_index<1>, unexpected<E>(std::forward<Args>(args)...)) {}

    expected& operator=(const expected&) = default;
    expected& operator=(expected&&) = default;

    template<typename G>
    expected& operator=(const unexpected<G>& e) {
        data_.template emplace<1>(unexpected<E>(e.error()));
        return *this;
    }

    template<typename G>
    expected& operator=(unexpected<G>&& e) {
        data_.template emplace<1>(unexpected<E>(std::move(e.error())));
        return *this;
    }

    constexpr bool has_value() const noexcept { return has_val(); }
    constexpr explicit operator bool() const noexcept { return has_val(); }

    constexpr void operator*() const noexcept {}
    
    constexpr void value() const {
        if (!has_val()) throw bad_expected_access(error());
    }

    constexpr const E& error() const& noexcept {
        return std::get<1>(data_).error();
    }
    constexpr E& error() & noexcept {
        return std::get<1>(data_).error();
    }
    constexpr const E&& error() const&& noexcept {
        return std::move(std::get<1>(data_).error());
    }
    constexpr E&& error() && noexcept {
        return std::move(std::get<1>(data_).error());
    }

    void emplace() noexcept {
        data_.template emplace<0>();
    }
};

} // namespace coroute

#endif // coroute_HAS_STD_EXPECTED
