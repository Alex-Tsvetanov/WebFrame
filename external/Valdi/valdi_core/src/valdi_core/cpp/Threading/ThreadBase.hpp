//
//  ThreadBase.hpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 5/26/21.
//

#pragma once

#include <cstdint>

namespace Valdi {

using ThreadId = uint32_t;

constexpr ThreadId kThreadIdNull = 0;

ThreadId getCurrentThreadId();

} // namespace Valdi
