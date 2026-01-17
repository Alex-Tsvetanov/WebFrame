//
//  ExceptionTracker.hpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 2/23/23.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Constants.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {

class ExceptionTracker : public snap::NonCopyable {
public:
    virtual ~ExceptionTracker();

    constexpr explicit operator bool() const noexcept {
        return VALDI_LIKELY(_isEmpty);
    }

    void onError(Error&& error);
    void onError(const Error& error);
    void onError(std::string_view errorMessage);
    void onError(const char* errorMessage);

    Error getError() const;
    Error extractError();
    void clearError();

    template<typename T>
    Result<T> toResult(T&& value) {
        if (*this) {
            return Result<T>(std::forward<T>(value));
        } else {
            return extractError();
        }
    }

protected:
    void setEmpty(bool isEmpty);

    virtual Error onGetError() const = 0;
    virtual void onClearError() = 0;
    virtual void onSetError(Error&& error) = 0;

    void assertEmpty() const;
    void assertNotEmpty() const;

private:
    bool _isEmpty = true;
};

class SimpleExceptionTracker final : public ExceptionTracker {
public:
    SimpleExceptionTracker();
    ~SimpleExceptionTracker() final;

protected:
    Error onGetError() const final;
    void onClearError() final;
    void onSetError(Error&& error) final;

private:
    std::optional<Error> _error;
};

} // namespace Valdi
