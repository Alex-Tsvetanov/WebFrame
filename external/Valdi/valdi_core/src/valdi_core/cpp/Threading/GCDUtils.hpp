//
//  GCDUtils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/15/19.
//

#if __APPLE__

#include "valdi_core/cpp/Threading/Thread.hpp"
#include <dispatch/dispatch.h>

namespace Valdi {

qos_class_t ValdiQoSClassToGCD(ThreadQoSClass qosClass);

} // namespace Valdi

#endif
