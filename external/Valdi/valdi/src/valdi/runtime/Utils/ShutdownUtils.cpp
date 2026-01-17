//
//  ShutdownUtils.cpp
//  valdi
//
//  Created by Simon Corsin on 9/24/21.
//

#include "valdi/runtime/Utils/ShutdownUtils.hpp"
#include <atomic>

namespace Valdi {

std::atomic_bool kApplicationShuttingDown = false;

bool isApplicationShuttingDown() {
    return kApplicationShuttingDown.load();
}

void setApplicationShuttingDown(bool applicationShuttingDown) {
    kApplicationShuttingDown = applicationShuttingDown;
}

} // namespace Valdi
