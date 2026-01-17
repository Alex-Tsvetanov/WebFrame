//
//  Result.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#pragma once

#if defined(__EXCEPTIONS)
#undef __EXCEPTIONS
#endif

#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/Void.hpp"

namespace Valdi {

class ResultBase {
public:
    ResultBase() noexcept;

    ResultBase(const ResultBase& other) = delete;
    ResultBase(ResultBase&& other) = delete;

    bool success() const noexcept;

    bool failure() const noexcept;

    bool empty() const noexcept;

    void ensureSuccess() const noexcept;
    void ensureFailure() const noexcept;

    explicit operator bool() const noexcept;

    ResultBase& operator=(const ResultBase& other) = delete;

    ResultBase& operator=(ResultBase&& other) = delete;

protected:
    static constexpr size_t kUninitializedIndex = 0;
    static constexpr size_t kValueIndex = 1;
    static constexpr size_t kErrorIndex = 2;
    size_t _index;

    explicit ResultBase(size_t index) noexcept;

    static const char* getDescriptionFromError(const Error& error);
};

template<typename T>
class Result : public ResultBase {
public:
    // NOLINTNEXTLINE(modernize-use-equals-default)
    using ResultBase::ResultBase;

    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(T&& value) noexcept : ResultBase(kValueIndex) {
        new (&_storage) T(std::forward<T>(value));
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(const T& value) noexcept : ResultBase(kValueIndex) {
        new (&_storage) T(value);
    }

    Result(const Result<T>& other) noexcept : ResultBase(other._index) {
        if (_index == kValueIndex) {
            new (&_storage) T(other.unsafeGetValue());
        } else if (_index == kErrorIndex) {
            new (&_storage) Error(other.unsafeGetError());
        }
    }

    Result(Result<T>&& other) noexcept : ResultBase(other._index), _storage(other._storage) {
        other._index = kUninitializedIndex;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(Error&& error) noexcept : ResultBase(kErrorIndex) {
        new (&_storage) Error(std::move(error));
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(const Error& error) noexcept : ResultBase(kErrorIndex) {
        new (&_storage) Error(error);
    }

    ~Result() {
        deconstructStorage();
    }

    Result<T>& operator=(const Result<T>& other) noexcept {
        if (this != &other) {
            deconstructStorage();

            _index = other._index;

            if (_index == kValueIndex) {
                new (&_storage) T(other.unsafeGetValue());
            } else if (_index == kErrorIndex) {
                new (&_storage) Error(other.unsafeGetError());
            }
        }

        return *this;
    }

    Result<T>& operator=(Result<T>&& other) noexcept {
        if (this != &other) {
            deconstructStorage();

            _index = other._index;
            _storage = other._storage;

            other._index = kUninitializedIndex;
        }

        return *this;
    }

    T&& moveValue() noexcept {
        ensureSuccess();
        return std::move(unsafeGetValue());
    }

    const T& value() const noexcept {
        ensureSuccess();
        return unsafeGetValue();
    }

    T& value() noexcept {
        ensureSuccess();
        return unsafeGetValue();
    }

    /**
     Returns a description of the result, suitable for use in asserts.
     */
    const char* description() const noexcept {
        if (success()) {
            return "Result is success";
        }

        if (failure()) {
            return ResultBase::getDescriptionFromError(error());
        }

        return "Result is empty";
    }

    Error&& moveError() noexcept {
        ensureFailure();
        return std::move(unsafeGetError());
    }

    const Error& error() const noexcept {
        ensureFailure();
        return unsafeGetError();
    }

    template<typename Out, typename MapFunc>
    Result<Out> map(MapFunc&& mapFunc) const noexcept {
        if (success()) {
            return Result<Out>(mapFunc(value()));
        } else {
            return Result<Out>(error());
        }
    }

    template<typename Out>
    Result<Out> map() const noexcept {
        return map<Out>([](const auto& value) { return Out(value); });
    }

private:
    typename std::aligned_union<0, T, Error>::type _storage;

    Error& unsafeGetError() noexcept {
        return *reinterpret_cast<Error*>(&_storage);
    }

    const Error& unsafeGetError() const noexcept {
        return *reinterpret_cast<const Error*>(&_storage);
    }

    T& unsafeGetValue() noexcept {
        return *reinterpret_cast<T*>(&_storage);
    }

    const T& unsafeGetValue() const noexcept {
        return *reinterpret_cast<const T*>(&_storage);
    }

    void deconstructStorage() {
        if (_index == kValueIndex) {
            unsafeGetValue().~T();
        } else if (_index == kErrorIndex) {
            unsafeGetError().~Error();
        }
    }
};

} // namespace Valdi
