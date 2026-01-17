//
//  Thread.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/11/19.
//

#pragma once

#include <cstdint>

namespace Valdi {

enum ThreadQoSClass : uint8_t {
    ThreadQoSClassLowest,
    ThreadQoSClassLow,
    ThreadQoSClassNormal,
    ThreadQoSClassHigh,
    ThreadQoSClassMax
};

}
