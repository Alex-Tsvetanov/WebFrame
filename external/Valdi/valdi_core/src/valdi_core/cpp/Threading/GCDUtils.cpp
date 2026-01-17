//
//  GCDUtils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/15/19.
//

#include "valdi_core/cpp/Threading/GCDUtils.hpp"

#if __APPLE__

namespace Valdi {

qos_class_t ValdiQoSClassToGCD(ThreadQoSClass qosClass) {
    switch (qosClass) {
        case ThreadQoSClassLowest:
            return QOS_CLASS_BACKGROUND;
        case ThreadQoSClassLow:
            return QOS_CLASS_UTILITY;
        case ThreadQoSClassNormal:
            return QOS_CLASS_DEFAULT;
        case ThreadQoSClassHigh:
            return QOS_CLASS_USER_INITIATED;
        case ThreadQoSClassMax:
            return QOS_CLASS_USER_INTERACTIVE;
    }
}

} // namespace Valdi

#endif
