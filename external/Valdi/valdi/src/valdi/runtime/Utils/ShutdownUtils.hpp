//
//  ShutdownUtils.hpp
//  valdi
//
//  Created by Simon Corsin on 9/24/21.
//

#pragma once

#include <atomic>

namespace Valdi {

bool isApplicationShuttingDown();
void setApplicationShuttingDown(bool applicationShuttingDown);

} // namespace Valdi
