//
//  SimpleAtomicCancelable.hpp
//  valdi_core-pc
//
//  Created by Ramzy Jaber on 3/16/22.
//

#pragma once

#include "valdi_core/Cancelable.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <atomic>
namespace Valdi {

class SimpleAtomicCancelable : public SharedPtrRefCountable, public snap::valdi_core::Cancelable {
public:
    SimpleAtomicCancelable();

    ~SimpleAtomicCancelable() override;

    bool wasCanceled();

    void cancel() override;

private:
    std::atomic_bool _wasCanceled;
};
} // namespace Valdi
