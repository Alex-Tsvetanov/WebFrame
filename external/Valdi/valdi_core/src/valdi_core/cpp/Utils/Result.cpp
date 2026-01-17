//
//  Result.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/5/19.
//

#include "valdi_core/cpp/Utils/Result.hpp"
#include "utils/debugging/Assert.hpp"

namespace Valdi {

ResultBase::ResultBase() noexcept : _index(kUninitializedIndex) {}

ResultBase::ResultBase(size_t index) noexcept : _index(index) {}

bool ResultBase::success() const noexcept {
    return _index == kValueIndex;
}

bool ResultBase::failure() const noexcept {
    return _index == kErrorIndex;
}

bool ResultBase::empty() const noexcept {
    return _index == kUninitializedIndex;
}

void ResultBase::ensureSuccess() const noexcept {
    SC_ASSERT(success());
}

void ResultBase::ensureFailure() const noexcept {
    SC_ASSERT(failure());
}

ResultBase::operator bool() const noexcept {
    return success();
}

const char* ResultBase::getDescriptionFromError(const Error& error) {
    thread_local static std::string tLastErrorDescription;

    tLastErrorDescription = error.toString();
    return tLastErrorDescription.c_str();
}

} // namespace Valdi
